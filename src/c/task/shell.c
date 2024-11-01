#include "../kernel/kernel.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/timer/timer.h"
#include "../drivers/serial_port/serial_port.h"
#include "../task/task.h"

#define BLACK_COLOR 0x0
#define GREEN_COLOR 0xa
#define YELLOW_COLOR 0xe

char *cmnds_names[] = {
    "help",
    "clear",
    "sleep",
    "list",
    "create",
    "edit",
    "read",
    "delete",
};

// Функція для пошуку команди за позицією тексту
char search_command(unsigned short *position_text)
{
    // Обчислення кількості елементів (команд) у масиві
    unsigned short cmnds_nums = sizeof(cmnds_names) / sizeof(cmnds_names[0]);   
    // Перевіряємо кожну команду
    for (unsigned short cmnd_indx = 0; cmnd_indx < cmnds_nums; cmnd_indx++)     
    {
        unsigned short pntr = *position_text;  // Збереження позиції тексту
        bool same = true;  // Прапорець для перевірки на збіг
        unsigned short cmnd_chrs_shift = 0;  // Зсув символів команди
        // Порівняння символів команди з введеним текстом
        while(*(cmnds_names[cmnd_indx] + cmnd_chrs_shift) != '\0' && pntr < cursor_pointer && same)
        {
            // Перевірка на збіг символів
            if (*(cmnds_names[cmnd_indx] + cmnd_chrs_shift) != *(framebuffer + pntr * 2))
                same = false;  // Якщо символи не збігаються
            
            cmnd_chrs_shift++;  // Зсув на наступний символ
            pntr++;  // Зсув позиції тексту
        }

        // Перевірка на завершення команди
        if (same)
        {
            if (*(cmnds_names[cmnd_indx] + cmnd_chrs_shift) != '\0'){} // Якщо не завершено

            else if(pntr == cursor_pointer || framebuffer[pntr * 2] == ' ')  // Якщо досягнуто кінця тексту
            {
                *position_text = pntr;  // Оновлення позиції
                return cmnd_indx;  // Повернення індексу команди
            }
        }
    }

    return -1;  // Якщо команда не знайдена
};

// Функція для очищення екрану
void cmnd_clear()
{
    clean_screen();  // Очищення екрану
    cursor_pointer = 0;  // Скидання курсору
    char *ver_msg = "KlymOS 0.1\n";  // Повідомлення з версією системи
    print(ver_msg, BLACK_COLOR, GREEN_COLOR);  // Виведення повідомлення
}

// Функція для виведення довідки
void cmnd_help()
{
    print(new_line, BLACK_COLOR, BLACK_COLOR);  // Виведення нового рядка
    // Повідомлення з доступними командами
    char *msg = "\t   ======== Welcome to the system! ========\n\t   == You can use the following commands ==\n\t   help - show help message\n\t   clear - clean shell\n\t   sleep - screensave mode on\n\t   list - show tree of files\n\t   create - create new file\n\t   edit - edit the file\n\t   read - show file content\n\t   delete - delete file\n";
    print(msg, BLACK_COLOR, YELLOW_COLOR);  // Виведення довідки
}

// Функція для активації режиму сну
void cmnd_sleep()
{
    TIMER_TICKES = 1001;  // Встановлення значення таймера
    KEY_PRESSED = 0;  // Скидання прапорця натискання клавіші
}

// Функція для виведення списку файлів
void cmnd_list()
{
    print(new_line, BLACK_COLOR, BLACK_COLOR);  // Виведення нового рядка
    // Виведення файлів у списку
    for (unsigned char slot_indx = 0; slot_indx < files_count; slot_indx++)
    {
        // Виведення пробілів перед кожним файлом
        for (unsigned char cntr = 0; cntr < line_st_ofst; cntr++)
        {
            char *msg_space = " ";
            print(msg_space, BLACK_COLOR, BLACK_COLOR);
        }

        char *msg = "-> ";  // Позначення файлу
        print(msg, BLACK_COLOR, YELLOW_COLOR);  // Виведення позначення

        // Перевірка, чи файл існує
        if (files_slots[slot_indx][0] == 1)
        {
            unsigned char shift_name = 0;  // Зсув для ім'я файлу
            print(&files_names[slot_indx][shift_name], BLACK_COLOR, YELLOW_COLOR);  // Виведення імені файлу
        }

        print(new_line, BLACK_COLOR, YELLOW_COLOR);  // Виведення нового рядка
    }
}

// Функція для створення нового файлу
void cmnd_create(unsigned short *pntr_aftr_cmnd)
{
    unsigned short pntr = *pntr_aftr_cmnd;  // Збереження позиції після команди
    find_param_start(&pntr);  // Знаходження початку параметрів
    unsigned char parameter[75];  // Масив для зберігання параметрів
    // Перевірка наявності параметрів
    if(pntr == *pntr_aftr_cmnd || pntr == cursor_pointer) // Якщо не введено параметри
    {
        char *msg = "There is no parameter given\n";  // Повідомлення про відсутність параметрів
        print_info_cmd(msg);  // Виведення повідомлення
    }
    else if (pntr < cursor_pointer)  // Якщо введено параметри
    {
        read_param(&pntr, &parameter);  // Зчитування параметрів

        bool same_name_file_exist = false;  // Флаг для перевірки наявності файлу з таким ім'ям
        unsigned char same_name_file_indx = 0;   // Індекс наявного файлу з таким ім'ям

        find_file_name(&same_name_file_exist, &parameter, &same_name_file_indx);  // Пошук файлу

        // Якщо ім'я не збігається і є новим
        if (!same_name_file_exist) 
        {
            bool free_slot_exist = false;  // Флаг для перевірки наявності вільного слота
            unsigned char free_slot = 0;  // Індекс вільного слота
            for (unsigned char slot_indx = 0; slot_indx < files_count && !free_slot_exist; slot_indx++)
            {
                // Пошук вільного слота
                if (files_slots[slot_indx][0] == 0)
                {
                    free_slot_exist = true;  // Знайдено вільний слот
                    free_slot = slot_indx;  // Запам'ятовуємо індекс вільного слота
                }
            }

            if (!free_slot_exist) // Якщо вільного слоту немає
            {
                char *msg = "There are no slots available for new file\n";  // Повідомлення про відсутність вільних слотів
                print_info_cmd(msg);  // Виведення повідомлення
            }
            else // Якщо вільний слот є, та створюємо новий файл
            {
                unsigned char shift_name = 0;  // Зсув для копіювання імені файлу
                // Копіюємо символи імені файлу
                while (parameter[shift_name] != '\0')   
                {
                    files_names[free_slot][shift_name] = parameter[shift_name];  // Копіювання імені файлу
                    shift_name++;
                }
                files_slots[free_slot][0] = 1; // Встановлюємо значення 1, щоб позначити слот як зайнятий
                print(new_line, BLACK_COLOR, BLACK_COLOR);  // Виведення нового рядка
                char *msg = "File created\n";  // Повідомлення про створення файлу
                print_info_cmd(msg);  // Виведення повідомлення
            }
        }
        else // Якщо файл з таким ім'ям вже існує
        {
            char *msg = "File with the same name already exists\n";  // Повідомлення про наявність файлу
            print_info_cmd(msg);  // Виведення повідомлення
        }
    }
}


// Функція для видалення файлу
void cmnd_delete(unsigned short *pntr_aftr_cmnd)
{
    char *new_line = "\n"; // Символ нового рядка
    unsigned short pntr = *pntr_aftr_cmnd; // Збереження позиції після команди

    // Знаходження початку параметрів команди
    find_param_start(&pntr);

    unsigned char parameter[75]; // Масив для зберігання параметра (імені файлу)

    // Перевірка наявності параметрів команди
    if(pntr == *pntr_aftr_cmnd || pntr == cursor_pointer) // Якщо немає зсуву - команда лише в кінці рядка
    {
        char *msg = "There is no parameter given\n"; // Повідомлення про відсутність параметрів
        print_info_cmd(msg); // Виведення повідомлення
    }
    else if (pntr < cursor_pointer) // Якщо параметри введені
    {
        read_param(&pntr, &parameter); // Зчитування параметрів (імені файлу)
        bool same_name_file_exist = false; // Флаг для перевірки наявності файлу
        unsigned char same_name_file_indx = 0; // Індекс для зберігання знайденого файлу
        find_file_name(&same_name_file_exist, &parameter, &same_name_file_indx); // Пошук файлу за його ім'ям

        if (same_name_file_exist) // Якщо файл знайдено
        {
            // Очищення вмісту файлу
            for (unsigned short shift_file = 0; shift_file < 2000; shift_file++)
            {
                files_content[same_name_file_indx][shift_file] = BLACK_COLOR; // Очистка вмісту файлу
            }
            // Очищення імені файлу
            for (unsigned char shift_name = 0; shift_name < 75; shift_name++)
            {
                files_names[same_name_file_indx][shift_name] = BLACK_COLOR; // Очищення імені файлу
            }
            // Оновлення індексу останнього символу файлу
            files_last_chr_indx[same_name_file_indx][0] = 0; // Скидання індексу
            files_slots[same_name_file_indx][0] = 0; // Встановлення слоту як вільного
            print(new_line, BLACK_COLOR, BLACK_COLOR); // Виведення нового рядка
            char *msg = "File was deleted\n"; // Повідомлення про успішне видалення
            print(msg, BLACK_COLOR, YELLOW_COLOR); // Виведення повідомлення
        }
        else // Якщо файл з таким ім'ям не знайдено
        {
            char *msg = "File with such a name does not exist\n"; // Повідомлення про ненайдене
            print_info_cmd(msg); // Виведення повідомлення
        }
    }
}


// Функція для читання вмісту файлу
void cmnd_read(unsigned short *pntr_aftr_cmnd)
{
    unsigned short pntr = *pntr_aftr_cmnd; // Збереження позиції після команди

    find_param_start(&pntr); // Знаходження початку параметрів

    unsigned char parameter[75]; // Масив для зберігання назви файлу

    // Перевірка, чи є введений параметр (ім'я файлу)
    if (pntr == *pntr_aftr_cmnd || pntr == cursor_pointer) // Якщо вказівник не переміщується - команда просто на кінці рядка
    {
        char *msg = "There is no parameter given\n"; // Повідомлення про відсутність параметра
        print_info_cmd(msg); // Виведення повідомлення
    }
    else if (pntr < cursor_pointer) // Якщо є пробіли, а після них є символи - зберігаємо параметр
    {
        read_param(&pntr, &parameter); // Зчитування параметра (імені файлу)

        // Питання: чи існує файл з такою назвою?

        bool same_name_file_exist = false; // Флаг наявності файлу з такою назвою
        unsigned char same_name_file_indx = 0; // Індекс знайденого файлу
        // Пошук файлу з такою назвою
        find_file_name(&same_name_file_exist, &parameter, &same_name_file_indx); // Виклик функції для пошуку

        if (same_name_file_exist) // Якщо файл знайдено - показуємо його вміст за збереженим індексом
        {
            print(new_line, BLACK_COLOR, BLACK_COLOR); // Виведення нового рядка перед вмістом файлу
            char *msg = &(files_content[same_name_file_indx]); // Отримання вмісту файлу за індексом
            print(msg, BLACK_COLOR, YELLOW_COLOR); // Виведення вмісту файлу

            print(new_line, BLACK_COLOR, BLACK_COLOR); // Виведення нового рядка після вмісту файлу
        }
        else // Якщо файл з такою назвою не існує
        {
            char *msg = "File with such a name does not exist\n"; // Повідомлення про ненайдене
            print_info_cmd(msg); // Виведення повідомлення
        }
    }
}

// Функція для редагування вмісту файлу
void cmnd_edit(unsigned short *pntr_aftr_cmnd)
{
    unsigned short pntr = *pntr_aftr_cmnd; // Збереження позиції після команди

    find_param_start(&pntr); // Знаходження початку параметрів

    unsigned char parameter[75]; // Масив для зберігання назви файлу

    // Перевірка, чи є введений параметр (ім'я файлу)
    if (pntr == *pntr_aftr_cmnd || pntr == cursor_pointer) // Якщо вказівник не переміщується - команда просто на кінці рядка або були пробіли, але параметра немає
    {
        print(new_line, BLACK_COLOR, BLACK_COLOR); // Виведення нового рядка
        char *msg = "There is no parameter given\n"; // Повідомлення про відсутність параметра
        print_info_cmd(msg); // Виведення повідомлення
        print(line_char, BLACK_COLOR, YELLOW_COLOR); // Виведення символу рядка для відокремлення
    }
    else if (pntr < cursor_pointer) // Якщо є пробіли, а після них є символи - зберігаємо параметр
    {
        read_param(&pntr, &parameter); // Зчитування параметра (імені файлу)

        bool same_name_file_exist = false; // Флаг наявності файлу з такою назвою
        unsigned char same_name_file_indx = 0; // Індекс знайденого файлу
        find_file_name(&same_name_file_exist, &parameter, &same_name_file_indx); // Виклик функції для пошуку

        if (same_name_file_exist) // Якщо файл знайдено - переходимо в режим редагування
        {
            edit_mode = true; // Увімкнення режиму редагування
            save_console_state(); // Збереження поточного стану консолі
            clean_screen(); // Очищення екрану
            file_slot_indx = same_name_file_indx; // Збереження індексу файлу для редагування

            char *msg = &(files_content[same_name_file_indx]); // Отримання вмісту файлу за індексом
            print(msg, BLACK_COLOR, YELLOW_COLOR); // Виведення вмісту файлу

            files_content[same_name_file_indx][files_last_chr_indx[file_slot_indx][0]] = BLACK_COLOR; // Відзначення останнього символу
        }
        else // Якщо файл з такою назвою не існує
        {
            char *msg = "File with such a name does not exist\n"; // Повідомлення про ненайдене
            print_info_cmd(msg); // Виведення повідомлення
        }
    }
}
