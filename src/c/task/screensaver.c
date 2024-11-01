// Включає необхідні заголовочні файли
#include "../kernel/kernel.h" // Заголовок ядра
#include "../drivers/keyboard/keyboard.h" // Заголовок драйвера клавіатури
#include "../drivers/timer/timer.h" // Заголовок драйвера таймера
#include "../drivers/serial_port/serial_port.h" // Заголовок драйвера серійного порту
#include "../task/task.h" // Заголовок задач

// Визначення параметрів відеопам'яті
#define VIDEO_MEMORY 0xB8000 // Адреса відеопам'яті
#define VGA_WIDTH 80 // Ширина VGA екрана
#define MAX_ROWS 25 // Максимальна кількість рядків
#define MAX_COLS 80 // Максимальна кількість стовпців

// Визначення кольорів
#define BLACK_COLOR 0x0 // Чорний колір
#define GREEN_COLOR 0xa // Зелений колір
#define YELLOW_COLOR 0xe // Жовтий колір
#define RED_COLOR 0xc // Червоний колір
#define WHITE_COLOR 0xf // Білий колір
#define PINK_COLOR 0xd // Рожевий колір
#define LIGHT_GREEN_COLOR 0xa // Світло-зелений колір
#define BLUE_COLOR 0x1 // Синій колір

// Масив для зберігання кольорів символів консолі (чорний фон, білий текст)
unsigned char cnsl_state_clrs[25 * 80];


// Функція для друку символу на вказаній позиції екрана
void print_char(int row, int col, char c) {
    unsigned short position = (row * VGA_WIDTH + col) * 2; // Обчислення позиції в буфері відеопам'яті
    volatile char *video = (volatile char *)VIDEO_MEMORY; // Вказівник на відеопам'ять
    video[position] = c; // Запис символу в відеопам'ять
    video[position + 1] = 0x0A; // Запис кольору (чорний фон, білий текст)
}

// Функція для очищення екрана
void clean_screen() {
    char *framebuff = (char *)0xb8000; // Вказівник на початок відеопам'яті
    // Очищення екрана шляхом заповнення пробілами
    for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
        framebuff[i] = ' '; // Запис пробілу
        framebuff[i + 1] = 0x07; // Запис кольору (білий текст на чорному фоні)
    }
}

// Функція для друку повідомлень на екрані з кольором фону та шрифту
void print(char *msg, unsigned char clr_bckg, unsigned char clr_fnt) {
    // Обробка кожного символу в повідомленні
    while (*msg != '\0') {
        if (*msg == '\n') // Якщо символ - новий рядок
            cursor_pointer += 80 - cursor_pointer % 80; // Перехід на початок наступного рядка
        else {
            // Запис символу та його кольору у відеопам'ять
            *(framebuffer + cursor_pointer * 2) = *msg; // Запис символу
            *(framebuffer + cursor_pointer * 2 + 1) = (clr_bckg << 4) | clr_fnt; // Запис кольору
            cursor_pointer++; // Переміщення вказівника на наступну позицію
        }
        msg++; // Перехід до наступного символу

        // Прокрутка екрана, якщо вказівник виходить за межі
        if (cursor_pointer > 1999)  
            scroll(); 
        else
            put_cursor(cursor_pointer); // Оновлення позиції курсора
    }
}

// Функція для прокрутки вмісту екрана вгору
void scroll() {
    // Цикл для перенесення кожного рядка вгору на один рядок
    for (unsigned char rw = 0; rw < 24; rw++) {
        for (unsigned char clmn = 0; clmn < 80; clmn++) {
            // Копіюємо символи з рядка нижче на рядок вище
            *(framebuffer + rw * (80 * 2) + clmn * (2)) = 
            *(framebuffer + (rw + 1) * (80 * 2) + clmn * (2)); // Копіюємо символ
            *(framebuffer + rw * (80 * 2) + clmn * (2) + 1) = 
            *(framebuffer + (rw + 1) * (80 * 2) + clmn * (2) + 1); // Копіюємо кольори
        }
    }

    // Очищення останнього рядка (24-го) екрану
    for (unsigned char clmn = 0; clmn < 80; clmn++) {
        *(framebuffer + 24 * (80 * 2) + clmn * (2)) = 0x0; // Записуємо пробіл
        *(framebuffer + 24 * (80 * 2) + clmn * (2) + 1) = 0xf; // Білий текст на чорному фоні
    }
    
    cursor_pointer = 80 * 24; // Встановлюємо курсор на початок останнього рядка
    put_cursor(cursor_pointer); // Оновлюємо положення курсора на екрані
}



// Функція для відновлення стану консолі
void recover_console_state() {

    cursor_pointer = cnsl_state_pntr; // Відновлюємо положення курсора

    cnsl_state_pntr = 0; // Скидаємо покажчик на стан консолі

    put_cursor(cursor_pointer); // Оновлюємо положення курсора

    // Відновлення вмісту консолі з збереженого стану
    for (unsigned char row = 0; row < 25; row++) {
        for (unsigned char col = 0; col < 80; col++) {
            // Відновлюємо символи з збереженого стану
            *(framebuffer + row * (80 * 2) + col * (2)) = cnsl_state_chrs[row * 80 + col];
            cnsl_state_chrs[row * 80 + col] = BLACK_COLOR; // Скидаємо збережений символ

            // Відновлюємо кольори з збереженого стану
            *(framebuffer + row * (80 * 2) + col * (2) + 1) = cnsl_state_clrs[row * 80 + col];
            cnsl_state_clrs[row * 80 + col] = BLACK_COLOR; // Скидаємо збережений колір
        }
    }
}
// Функція для збереження тексту консолі
void save_console_txt() {
    if (!STARTED) { // Перевірка, чи анімація вже не запущена
        // Зберігаємо вміст екрану
        for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
            SAVE[i] = framebuff[i];       // Збереження символу в масив SAVE
            SAVE[i + 1] = framebuff[i + 1]; // Збереження кольору символу
            framebuff[i] = ' ';            // Очищення екрана, записуємо пробіли
            framebuff[i + 1] = 0x07;       // Встановлення білого тексту на чорному фоні
        }

        put_cursor(-1);  // Приховуємо курсор
        STARTED = true;  // Переходимо в режим анімації
    }
}

// Функція для початку анімації логотипу
void start_animation() {
    static int logo_col = MAX_COLS - 33;  // Позиція логотипу (початок з правого краю)
    static int logo_row = (MAX_ROWS / 2) - (13 / 2);  // Вертикальне центроване положення
    static int direction = 1;  // Напрямок для вертикального руху: 1 для вниз, -1 для вгору
    static int color_index = 0;  // Індекс для зміни кольору
    const unsigned char colors[] = {WHITE_COLOR, YELLOW_COLOR, LIGHT_GREEN_COLOR, RED_COLOR}; // Масив кольорів
    const char *logo[] = { // ASCII арт для логотипу BMW
        "##    ##",
        "  ####  ",
        "  ####  ",
        "##    ##"
    };
    int logo_height = 4;  // Висота логотипу 
    int logo_width = 9;  // Ширина логотипу 

    // Очищення попередньої позиції логотипу
    for (int row = 0; row < logo_height; row++) {
        for (int col = 0; col < logo_width; col++) {
            *(framebuff + ((logo_row + row) * MAX_COLS + (logo_col + col)) * 2) = ' '; // Очищуємо символ
            *(framebuff + ((logo_row + row) * MAX_COLS + (logo_col + col)) * 2 + 1) = BLACK_COLOR; // Очищуємо колір
        }
    }

    // Відображення логотипу ASCII арт на поточній позиції
    for (int row = 0; row < logo_height; row++) {
        for (int col = 0; col < logo_width; col++) {
            char character = logo[row][col];
            if (character != ' ') {  // Відображаємо тільки не-пробільні символи
                unsigned char color = colors[color_index];  // Вибір кольору з масиву
                *(framebuff + ((logo_row + row) * MAX_COLS + (logo_col + col)) * 2) = character; // Встановлюємо символ
                *(framebuff + ((logo_row + row) * MAX_COLS + (logo_col + col)) * 2 + 1) = color; // Встановлюємо колір
            }
        }
    }

    // Рух логотипу вліво, оновлюючи колонку
    logo_col--;

    // Скидаємо позицію на правий край, якщо логотип досяг лівої межі
    if (logo_col < -logo_width) {
        logo_col = MAX_COLS - 1; // Переміщуємо логотип назад на правий край
    }

    // Оновлення вертикальної позиції для створення ефекту плаваючого руху
    logo_row += direction;

    // Зміна напряму, якщо логотип досягає верхньої або нижньої межі екрана
    if (logo_row < 0 || logo_row > MAX_ROWS - logo_height) {
        direction *= -1;  // Зміна напряму руху
    }

    // Зміна кольору для наступного кадру
    color_index = (color_index + 1) % (sizeof(colors) / sizeof(colors[0]));  // Циклічно змінюємо колір
}


