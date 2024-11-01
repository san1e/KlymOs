#include "kernel/kernel.h"  // Підключаємо заголовок ядра
#include "drivers/keyboard/keyboard.h"  // Підключаємо драйвер клавіатури
#include "drivers/timer/timer.h"  // Підключаємо драйвер таймера
#include "drivers/serial_port/serial_port.h"  // Підключаємо драйвер серійного порту
#include "task/task.h"  // Підключаємо заголовок задач


// Обробник виключень
void exception_handler(u32 interrupt, u32 error, char *message) {
    serial_log(LOG_ERROR, message);  // Логування помилки
}

// Ініціалізація ядра
void init_kernel() {
    init_gdt();  // Ініціалізація глобальної таблиці дескрипторів
    init_idt();  // Ініціалізація таблиці дескрипторів переривань
    init_exception_handlers();  // Ініціалізація обробників виключень
    init_interrupt_handlers();  // Ініціалізація обробників переривань
    register_timer_interrupt_handler();  // Реєстрація обробника переривань таймера
    register_keyboard_interrupt_handler();  // Реєстрація обробника переривань клавіатури
    configure_default_serial_port();  // Налаштування стандартного серійного порту
    set_exception_handler(exception_handler);  // Встановлення обробника виключень
    enable_interrupts();  // Увімкнення переривань
}

// Функція для встановлення позиції курсора
void put_cursor(unsigned short pos) {
    out(0x3D4, 14);  // Вказуємо регістр позиції курсора
    out(0x3D5, ((pos >> 8) & 0x00FF));  // Високий байт позиції
    out(0x3D4, 15);  // Вказуємо регістр позиції курсора
    out(0x3D5, pos & 0x00FF);  // Низький байт позиції
}

// Безкінечний цикл зупинки
_Noreturn void halt_loop() {
    while (1) { halt(); }  // Безкінечний цикл для зупинки процесора
}

// -------------- КОМАНДИ --------------

// Массив вказівників на команди
void (*cmnds[])(unsigned short *) =   
{
    cmnd_help,  // Допомога
    cmnd_clear,  // Очистити екран
    cmnd_sleep,  // Заснути
    cmnd_list,  // Перелік
    cmnd_create,  // Створити
    cmnd_edit,  // Редагувати
    cmnd_read,  // Читати
    cmnd_delete,  // Видалити
};

// -------------- КОЛЬОРИ --------------

// Визначення кольорів
#define BLACK_COLOR 0x0  // Чорний колір
#define GREEN_COLOR 0xa  // Зелений колір
#define YELLOW_COLOR 0xe  // Жовтий колір
#define RED_COLOR 0xc  // Червоний колір
#define WHITE_COLOR 0xf  // Білий колір

int bcgr_color = BLACK_COLOR;  // Фоновий колір строки

// -------------- ОБМЕЖЕННЯ --------------

// Визначення обмежень для рядків та стовпців
#define MAX_ROWS 25  // Максимальна кількість рядків
#define MAX_COLS 80  // Максимальна кількість стовпців
#define MAX_COMMAND_LENGTH 100  // Максимальна довжина команди
#define MAX_DELAY 100  // Максимальна затримка перед анімацією

// -------------- ОБМЕЖЕННЯ ДЛЯ ФАЙЛІВ ТА РОБОТА З НИМИ --------------

// Змінні для роботи з файлами
bool edit_mode = false;  // Режим редагування
unsigned char file_slot_indx = 0;  // Індекс слоту файлу

// Масиви для зберігання імен файлів, стану слоту та вмісту
unsigned char files_names[10][75]; 
unsigned char files_slots[10][1];  // 0 - доступний, 1 - зайнятий
unsigned char files_content[10][2000];  // Масив для зберігання вмісту файлів
unsigned short files_last_chr_indx[10][1];  // Поточний індекс останнього символу у файлі
unsigned char files_count = 10;  // Кількість файлів

// -------------- РОБОТА В КОНСОЛІ --------------

// Вказівник на буфер консолі
char *framebuffer = (char *) 0xb8000; 
unsigned char *line_char = " $ ";  // Символи рядка
unsigned short cursor_pointer = 0;  // Вказівник курсора
unsigned char line_st_ofst = 3;  // Відступ для позиції курсора

// -------------- АНІМАЦІЯ --------------

// Змінні для анімації
char *framebuff = (char *) 0xb8000;  // Буфер для анімації
bool STARTED = false;  // Статус анімації
unsigned short current_row = 0, current_col = 0;  // Поточні рядок та стовпець
char SAVE[MAX_ROWS * MAX_COLS * 2];  // Для зберігання тексту
int TIMER_TICKES = 0;  // Кількість тикань таймера
int KEY_PRESSED = 0;  // Кількість натиснень клавіш
char *new_line = "\n";  // Новий рядок

// Обробник подій клавіатури
void key_handler(struct keyboard_event event) {
    if (event.key_character && event.type == EVENT_KEY_PRESSED) {  // Якщо клавіша натиснута
        KEY_PRESSED = TIMER_TICKES;  // Оновлюємо таймер натискання клавіші
        if (STARTED) { return; }  // Якщо анімація триває, виходимо

        if (edit_mode) {  // Якщо в режимі редагування
            if (event.key == KEY_BACKSPACE) {  // Обробка натискання Backspace
                if (cursor_pointer > 0) {
                    files_last_chr_indx[file_slot_indx][0]--;  // Зменшуємо індекс останнього символу
                    files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] == BLACK_COLOR;  // Очищаємо символ
                    if (files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] == '\n') {  // Якщо символ - новий рядок
                        cursor_pointer--; 
                        while (*(framebuffer + (cursor_pointer - 1) * 2) == BLACK_COLOR && cursor_pointer % 80 != 0) {  // Очищення символів
                            cursor_pointer--;
                        }
                    } else {
                        cursor_pointer--;
                    }

                    *(framebuffer + cursor_pointer * 2) = BLACK_COLOR;  // Очищення символу у буфері
                    put_cursor(cursor_pointer);  // Оновлення позиції курсора
                }
            } else if (event.key == KEY_ENTER) {  // Обробка натискання Enter
                if (cursor_pointer / 80 < 23) {  // Перевірка, чи не виходимо за межі
                    files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] = event.key_character;  // Зберігаємо символ
                    print(&event.key_character, bcgr_color, YELLOW_COLOR);  // Виведення символу
                    files_last_chr_indx[file_slot_indx][0]++;  // Збільшуємо індекс останнього символу
                }
            } else if (event.key == KEY_TAB) {  // Обробка натискання Tab
                files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] = '\0';  // Завершуємо вміст файлу
                edit_mode = false;  // Виходимо з режиму редагування
                clean_screen();  // Очищаємо екран
                recover_console_state();  // Відновлюємо стан консолі
                file_slot_indx = 0;  // Скидаємо індекс файлу
                char *new_line = "\n";  // Створюємо новий рядок
                print(new_line, BLACK_COLOR, BLACK_COLOR);  // Виводимо новий рядок
                print(line_char, BLACK_COLOR, GREEN_COLOR);  // Виводимо символи рядка
            } else {  // Для інших клавіш
                if (cursor_pointer % 80 < 79) {  // Якщо курсор не на межі
                    files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] = event.key_character;  // Зберігаємо символ
                    print(&event.key_character, bcgr_color, YELLOW_COLOR);  // Виводимо символ
                    files_last_chr_indx[file_slot_indx][0]++;  // Збільшуємо індекс останнього символу
                }
            }
        } else {  // Якщо не в режимі редагування
            if (event.key == KEY_BACKSPACE) {  // Обробка натискання Backspace
                if (cursor_pointer % 80 > line_st_ofst) {  // Якщо є символи для видалення
                    cursor_pointer -= 1;  // Зменшуємо позицію курсора
                    *(framebuffer + cursor_pointer * 2) = bcgr_color;  // Очищаємо символ у буфері
                    put_cursor(cursor_pointer);  // Оновлення позиції курсора
                }
            } else if (event.key == KEY_ENTER) {  // Обробка натискання Enter
                char cmnd_nmbr = 0;  // Номер команди
                if (cursor_pointer % 80 > line_st_ofst) {  // Якщо є текст
                    unsigned short position_text = cursor_pointer - cursor_pointer % 80 + line_st_ofst;  // Позиція початку тексту

                    if (*(framebuffer + position_text * 2) != bcgr_color) {  // Якщо текст не порожній
                        cmnd_nmbr = search_command(&position_text);  // Шукаємо команду

                        if (cmnd_nmbr > -1) {  // Якщо команда знайдена
                            cmnds[cmnd_nmbr](&position_text);  // Виконуємо команду
                        } else if (cmnd_nmbr == -1) {  // Якщо команда не знайдена
                            char *msg_no_cmnd = "Command not found\n";  // Повідомлення про відсутність команди
                            print_info_cmd(msg_no_cmnd);  // Виводимо повідомлення
                        }
                    }
                } else {
                    print(new_line, bcgr_color, BLACK_COLOR);  // Виводимо новий рядок
                }

                if (cmnd_nmbr != 5 && cmnd_nmbr != 2) {  // Якщо команда не є певними номерами
                    print(line_char, bcgr_color, GREEN_COLOR);  // Виводимо символи рядка
                }
            } else {  // Для інших клавіш
                if (cursor_pointer % 80 < 79) {  // Якщо курсор не на межі
                    print(&event.key_character, bcgr_color, WHITE_COLOR);  // Виводимо символ
                }
            }
        }
    }
}

// Обробник тактових переривань таймера
void timer_tick_handler() {
    TIMER_TICKES++;  // Збільшуємо лічильник тактів
    if (TIMER_TICKES - KEY_PRESSED > MAX_DELAY) {  // Якщо затримка перевищує максимальну
        save_console_txt();  // Зберігаємо текст консолі

        if (TIMER_TICKES % 5 == 0) {  // Кожні 5 тактів
            start_animation();  // Запускаємо анімацію
        }
    } else {
        if (STARTED) {  // Якщо анімація завершена
            for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {  // Відновлюємо вміст екрану
                framebuff[i] = SAVE[i];  // Відновлюємо символ
                framebuff[i + 1] = SAVE[i + 1];  // Відновлюємо колір
            }
            STARTED = false;  // Скидаємо анімацію
            TIMER_TICKES = 0;  // Скидаємо лічильник тактів
            KEY_PRESSED = 0;  // Скидаємо лічильник натиснень
        }
    }
}

// Основна функція ядра
void kernel_entry() {
    init_kernel();  // Ініціалізуємо ядро
    keyboard_set_handler(key_handler);  // Встановлюємо обробник клавіатури
    timer_set_handler(timer_tick_handler);  // Встановлюємо обробник таймера
    clean_screen();  // Очищаємо екран
    char *ver_msg = "KlymOS 0.1\n";  // Повідомлення про версію
    print(ver_msg, BLACK_COLOR, GREEN_COLOR);  // Виводимо повідомлення
    print(line_char, BLACK_COLOR, GREEN_COLOR);  // Виводимо символи рядка
    halt_loop();  // Зупиняємо виконання
}
