#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "stdio-text-protocol.h"

static char line[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint32_t line_length = 0;

// строка не поместилась в буфер целиком — команду выполнять нельзя
static bool line_overflow = false;

static command_t command;

void stdio_text_protocol_init(void)
{
    line[0] = '\0';
    line_length = 0;
    line_overflow = false;
}

// делит строку на слова на месте: пробелы становятся нулевыми символами,
// указатели name и argv ссылаются на начала слов в том же буфере
static void split(command_t *command)
{
    command->name = line;
    command->argc = 0;
    command->truncated = false;

    char *p = line;
    bool in_word = false;
    uint32_t word = 0;

    while (*p != '\0')
    {
        if (*p == ' ')
        {
            *p = '\0';
            in_word = false;
        }
        else if (!in_word)
        {
            in_word = true;

            if (word == 0)
            {
                command->name = p;
            }
            else if (word <= STDIO_TEXT_PROTOCOL_MAX_ARGS)
            {
                command->argv[word - 1] = p;
            }

            word++;
        }

        p++;
    }

    command->argc = (word > 0) ? word - 1 : 0;

    // слов больше, чем имя и MAX_ARGS аргументов — строка не разобрана целиком
    if (word > STDIO_TEXT_PROTOCOL_MAX_ARGS + 1)
    {
        command->truncated = true;
        command->argc = STDIO_TEXT_PROTOCOL_MAX_ARGS;
    }
}

const command_t *stdio_text_protocol_handle(void)
{
    int symbol = getchar_timeout_us(0);

    // читаем символ без ожидания; нет символа — нет команды
    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    // конец строки
    if (symbol == '\r' || symbol == '\n')
    {
        // пустую строку пропускаем
        if (line_length == 0 && !line_overflow)
        {
            return NULL;
        }

        putchar('\n');

        if (line_length > 0)
        {
            split(&command);

            if (line_overflow)
            {
                command.truncated = true;
            }
        }
        else
        {
            // буфер был переполнен ещё в середине строки
            command.name = line;
            command.argc = 0;
            command.truncated = true;
        }

        line_overflow = false;

        // готовим буфер к следующей строке;
        // команда живёт до следующего вызова: её слова лежат в этом же буфере
        line_length = 0;

        return &command;
    }

    // Backspace (0x08 или 0x7f): убираем последний символ и стираем его на экране
    if (symbol == '\b' || symbol == 0x7f)
    {
        if (line_length > 0)
        {
            line_length--;
            line[line_length] = '\0';
            printf("\b \b");
        }

        return NULL;
    }

    // печатный символ: кладём в буфер и повторяем в терминал
    if (symbol >= 0x20 && symbol <= 0x7e)
    {
        if (line_length + 1 < STDIO_TEXT_PROTOCOL_LINE_SIZE)
        {
            line[line_length] = (char)symbol;
            line_length++;
            line[line_length] = '\0';
            putchar(symbol);
        }
        else
        {
            // буфер полон: строка не влезет целиком
            line_overflow = true;
            putchar(symbol);
        }
    }

    // прочие символы игнорируем
    return NULL;
}
