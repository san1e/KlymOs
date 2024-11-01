#include "../kernel/kernel.h" // Підключення заголовка ядра
#include "../drivers/keyboard/keyboard.h" // Підключення заголовка клавіатури
#include "../drivers/timer/timer.h" // Підключення заголовка таймера
#include "../drivers/serial_port/serial_port.h" // Підключення заголовка серійного порту
#include "../task/task.h" // Підключення заголовка для роботи з задачами

// Визначення кольорів для консолі
#define BLACK_COLOR 0x0
#define GREEN_COLOR 0xa
#define YELLOW_COLOR 0xe
#define RED_COLOR 0xc
#define WHITE_COLOR 0xf
#define PINK_COLOR 0xd

// Масиви для збереження стану консолі
unsigned char cnsl_state_chrs[25 * 80]; // Зберігає символи консолі
unsigned char cnsl_state_clrs[25 * 80]; // Зберігає кольори символів консолі
unsigned short cnsl_state_pntr = 0; // Вказівник на позицію курсора в консолі

// Функція для збереження стану консолі
void save_console_state() {
    // Зберігаємо поточний вказівник курсора
    cnsl_state_pntr = cursor_pointer;

    // Скидаємо вказівник курсора
    cursor_pointer = 0;

    // Встановлюємо курсор на нову позицію
    put_cursor(cursor_pointer);

    // Проходимо по всім рядкам і стовпцям консолі
    for (unsigned char row = 0; row < 25; row++) {
        for (unsigned char col = 0; col < 80; col++) {
            // Зберігаємо символи в масив
            cnsl_state_chrs[row * 80 + col] = 
                *(framebuffer + row * (80 * 2) + col * (2));

            // Зберігаємо кольори в масив
            cnsl_state_clrs[row * 80 + col] = 
                *(framebuffer + row * (80 * 2) + col * (2) + 1);
        }
    }
}

// Функція для знаходження початку параметра в консолі
void find_param_start(unsigned short *pntr) {
    // Продовжуємо до тих пір, поки символ пробіл та вказівник менше ніж курсор
    while (*(framebuffer + (*pntr) * 2) == ' ' && (*pntr) < cursor_pointer) {
        // Змінюємо колір пробілу на білий
        *(framebuffer + (*pntr) * 2 + 1) = WHITE_COLOR;
        (*pntr)++; // Збільшуємо вказівник
    }
}

// Функція для перевірки наявності файлу з таким же ім'ям
void find_file_name(bool *same_name_file_exist, unsigned char *parameter, unsigned char *same_name_file_indx) {
    for (unsigned char file_indx = 0; file_indx < files_count && !(*same_name_file_exist); file_indx++) {
        unsigned char shift_name = 0;

        // Перевірка кожного символу на збіг
        while (files_names[file_indx][shift_name] == parameter[shift_name] &&
               files_names[file_indx][shift_name] != '\0' &&
               parameter[shift_name] != '\0') {
            shift_name++; // Переходимо до наступного символу
        }

        // Якщо обидва рядки закінчилися, імена однакові
        if (files_names[file_indx][shift_name] == '\0' && parameter[shift_name] == '\0') {
            *same_name_file_exist = true; // Встановлюємо прапорець, що файл існує
            *same_name_file_indx = file_indx; // Запам'ятовуємо індекс файлу
        }
    }
}

// Функція для читання параметра з консолі
void read_param(unsigned short *pntr, unsigned char *parameter) {
    unsigned char param_indx = 0;
    // Читаємо символи до пробілу або до кінця рядка
    while (*(framebuffer + (*pntr) * 2) != ' ' && (*pntr) < cursor_pointer) {
        parameter[param_indx] = *(framebuffer + (*pntr) * 2); // Зберігаємо символ у параметрі

        param_indx++; // Збільшуємо індекс для наступного символу
        (*pntr)++; // Переходимо до наступної позиції
    }
    parameter[param_indx] = '\0'; // Завершуємо рядок нульовим символом
}

// Функція для виводу інформації про команду
void print_info_cmd(char *msg) {
    int i = 0;
    // Обробка кожного символу (можливо, для діагностики)
    while (msg[i] != '\0') {
        // Тут можна додати логіку для діагностики кожного символа, якщо необхідно
        i++;
    }

    // Виводимо повідомлення з кольоровим оформленням
    print(new_line, bcgr_color, BLACK_COLOR);
    print(msg, bcgr_color, RED_COLOR);
}
