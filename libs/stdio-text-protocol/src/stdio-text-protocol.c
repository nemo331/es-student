#include <stdio.h>
#include "stdio-text-protocol/stdio-text-protocol.h"

#include "pico/stdlib.h"


// буфер на 64 байта: 63 символа строки и нулевой в конце
static char line[STDIO_TEXT_PROTOCOL_LINE_SIZE];

// сколько символов уже в буфере
static uint32_t length = 0;

// строка не влезла в буфер или в неё не влезли все аргументы
static bool overflow = false;

// команда, которую возвращает handle: живёт до следующего вызова
static command_t command;

void stdio_text_protocol_init(void)
{
    length = 0;
    overflow = false;
}

// делит строку на слова на месте: пробелы замещаем нулями.
// Первое слово — имя команды, остальные — аргументы
static void split(void)
{
    line[length] = '\0';

    uint32_t i = 0;

    // первое слово — имя команды
    while (i < length && line[i] == ' ')
    {
        line[i] = '\0';
        i++;
    }

    uint32_t name_start = i;

    while (i < length && line[i] != ' ')
    {
        i++;
    }

    command.argc = 0;
    command.truncated = overflow;

    if (name_start < length)
    {
        command.name = &line[name_start];
    }
    else
    {
        command.name = "";
    }

    // остальные слова — аргументы
    while (i < length)
    {
        while (i < length && line[i] == ' ')
        {
            line[i] = '\0';
            i++;
        }

        if (i >= length)
        {
            break;
        }

        if (command.argc < STDIO_TEXT_PROTOCOL_MAX_ARGS)
        {
            command.argv[command.argc] = &line[i];
        }
        else
        {
            command.truncated = true;
        }
        command.argc++;

        while (i < length && line[i] != ' ')
        {
            i++;
        }
    }
}

const command_t *stdio_text_protocol_handle(void)
{
    int symbol = getchar_timeout_us(0);

    // нет символа — команда не готова
    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    // конец строки: пустую строку пропускаем, иначе печатаем перевод
    // строки, делим строку на слова и возвращаем команду
    if (symbol == '\r' || symbol == '\n')
    {
        if (length == 0 && !overflow)
        {
            return NULL;
        }

        putchar('\n');

        if (!overflow)
        {
            split();
        }
        else
        {
            command.argc = 0;
            command.name = "";
            command.truncated = true;
        }

        length = 0;
        overflow = false;

        return &command;
    }

    // Backspace (0x08 или 0x7f): убираем последний символ и стираем его
    if (symbol == 0x08 || symbol == 0x7f)
    {
        if (length > 0)
        {
            length--;
            printf("\b \b");
        }
        return NULL;
    }

    // печатный символ: кладём в буфер и повторяем в терминал;
    // буфер полон — помечаем строку как переполненную
    if (symbol >= 0x20 && symbol <= 0x7e)
    {
        if (length + 1 < STDIO_TEXT_PROTOCOL_LINE_SIZE)
        {
            line[length] = (char)symbol;
            length++;
            putchar(symbol);
        }
        else
        {
            overflow = true;
        }
    }

    return NULL;
}
