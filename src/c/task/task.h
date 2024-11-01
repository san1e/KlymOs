#ifndef TASK_H
#define TASK_H

// Включення необхідних заголовних файлів
#include "../kernel/kernel.h"          // Заголовок для ядра системи
#include "../drivers/keyboard/keyboard.h" // Заголовок для обробки введення з клавіатури
#include "../drivers/timer/timer.h"        // Заголовок для роботи з таймерами
#include "../drivers/serial_port/serial_port.h" // Заголовок для роботи з серійними портами

// Константи для максимальних розмірів консольного виводу
#define MAX_ROWS 25         // Максимальна кількість рядків у консолі
#define MAX_COLS 80         // Максимальна кількість стовпців у консолі

// Зовнішні змінні, оголошені в інших файлах
extern char *framebuffer;                  // Вказівник на буфер кадрів для виводу
extern unsigned short cursor_pointer;      // Вказівник на позицію курсора у буфері
extern int TIMER_TICKES;                   // Кількість таймерних тактів
extern int KEY_PRESSED;                    // Флаг, що вказує, чи була натиснута клавіша

extern char *framebuff;                    // Можливо, ще один вказівник на буфер кадрів (можливо, зайвий)

extern bool STARTED;                       // Прапорець, що вказує, чи програма запущена
extern bool edit_mode;                     // Прапорець, що вказує, чи активний режим редагування
extern unsigned char file_slot_indx;      // Індекс, що вказує на слот файлу
extern int bcgr_color;                     // Колір фону для консолі

extern char *new_line;                     // Вказівник для управління новими рядками
extern unsigned short cnsl_state_pntr;    // Вказівник на стан консолі
extern unsigned char cnsl_state_chrs[25 * 80]; // Масив для зберігання символів консолі

extern char SAVE[MAX_ROWS * MAX_COLS * 2]; // Буфер для зберігання вмісту консолі

// Масиви для управління файлами
extern unsigned char files_names[10][75];       // Імена файлів (до 10 файлів, по 75 символів)
extern unsigned char files_slots[10][1];        // Слоти для файлів (управлінська інформація)
extern unsigned char files_content[10][2000];    // Вміст файлів (до 2000 символів на файл)
extern unsigned short files_last_chr_indx[10][1]; // Останній індекс символу в кожному файлі
extern unsigned char files_count;                 // Кількість файлів, які зараз управляються

extern unsigned char line_st_ofst;               // Зміщення для початку рядка
extern unsigned char *line_char;                  // Вказівник на символи поточного рядка

// Функції для управління курсором та виводу
extern void put_cursor(unsigned short pos);            // Функція для встановлення позиції курсора
extern void print_char(int row, int colum, char c);   // Функція для виводу символа на певній позиції


// Функція для очищення екрану
extern void clean_screen();

// Функція для виводу повідомлень
extern void print(char *msg, unsigned char clr_bckg, unsigned char clr_fnt); // Вивід повідомлення з кольором фону та шрифту

// Командні функції
extern void cmnd_clear();                // Команда для очищення консолі
extern void cmnd_help();                 // Команда для виводу довідки
extern void cmnd_sleep();                // Команда для паузи виконання
extern void cmnd_list();                 // Команда для списку доступних файлів

// Функції для виводу інформаційних повідомлень
extern void print_info_cmd(char *msg);

// Функції для управління файлами
extern void cmnd_edit(unsigned short *pntr_aftr_cmnd);   // Команда для редагування файлу
extern void cmnd_read(unsigned short *pntr_aftr_cmnd);    // Команда для читання файлу
extern void cmnd_delete(unsigned short *pntr_aftr_cmnd);  // Команда для видалення файлу
extern void cmnd_create(unsigned short *pntr_aftr_cmnd);  // Команда для створення нового файлу

// Функція для пошуку команди
extern char search_command(unsigned short *position_text);


// Функція для початку анімації
extern void start_animation();


#endif // Завершення охорони заголовка
