#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "pico/stdlib.h"
#include "api.h"
#include "device.h"
#include "profiling.h"

// тип обработчика: команда приходит целиком
typedef void (*api_callback_t)(const command_t *command);

// строка таблицы: имя, справка и обработчик
typedef struct
{
    const char *name;
    const char *help;
    api_callback_t callback;
} api_command_t;

// ---- обработчики ----

static void cmd_info(const command_t *command)
{
    // паспорт устройства
    device_info();
}

static void cmd_uptime(const command_t *command)
{
    // монотонное время с момента сброса, в миллисекундах
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

// прикидка: за член ряда 4 операции с double, 175 + 110 + 190 + 110 = 585 тактов;
// 1 000 000 членов по 585 тактов при 125 МГц — около 4,7 с
const uint CALC_PI_TERMS = 1000000;

static double calc_pi(uint terms)
{
    // сумма ряда и знак очередного члена, оба double
    double sum = 0.0;
    double sign = 1.0;

    for (uint k = 0; k < terms; k++)
    {
        sum += sign / (2 * k + 1);
        sign = -sign;
    }

    // сумма ряда, умноженная на 4
    return sum * 4.0;
}

volatile double pi_result;

static bool parse_u32(const char *text, uint32_t *value)
{
    if (text == NULL || *text == '\0')
    {
        return false;
    }

    errno = 0;
    char *end = NULL;
    unsigned long number = strtoul(text, &end, 10);

    // ничего не разобрано, после числа остался мусор или число не влезло в long
    if (end == text || *end != '\0' || errno == ERANGE)
    {
        return false;
    }

    *value = (uint32_t)number;
    return true;
}

static void cmd_calc_pi(const command_t *command)
{
    // число членов ряда — необязательный аргумент
    uint32_t terms = CALC_PI_TERMS;

    if (command->argc > 1)
    {
        // лишние аргументы
        printf("error: usage calc_pi [terms]\n");
        return;
    }

    if (command->argc == 1)
    {
        if (!parse_u32(command->argv[0], &terms))
        {
            // неверная запись числа
            printf("error: usage calc_pi [terms]\n");
            return;
        }

        if (terms == 0)
        {
            printf("error: terms must be greater than zero\n");
            return;
        }
    }

    uint64_t start_us = time_us_64();
    pi_result = calc_pi(terms);
    uint64_t spent_us = time_us_64() - start_us;

    printf("pi: %.9f\n", pi_result);
    printf("time: %llu ms\n", spent_us / 1000);
}

static void cmd_main_time_exec(const command_t *command)
{
    printf("iteration avg %.2f us, max %u us\n",
           profiling_avg_us(), (unsigned)profiling_max_us());
}

static void cmd_main_time_reset(const command_t *command)
{
    profiling_reset_max();
    printf("max reset\n");
}

// ---- разбор числа ----

// разбирает десятичное число целиком: без хвоста, без переполнения, без пустой строки


// ---- таблица команд ----

static const api_command_t api_commands[] = {
    { "info",            "print device passport",            cmd_info },
    { "uptime",          "print milliseconds since reset",   cmd_uptime },
    { "calc_pi",         "compute pi by Leibniz series; optional arg: number of terms", cmd_calc_pi },
    { "main_time_exec",  "print superloop iteration average and maximum time", cmd_main_time_exec },
    { "main_time_reset", "reset iteration maximum",          cmd_main_time_reset },
};

#define API_COMMAND_COUNT (sizeof(api_commands) / sizeof(api_commands[0]))

void api_handle(const command_t *command)
{
    // команда принята не целиком — выполнять нельзя
    if (command->truncated)
    {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }

    // help выполняется сам: справка берётся из таблицы
    if (strcmp(command->name, "help") == 0)
    {
        printf("help: print this list\n");

        for (uint i = 0; i < API_COMMAND_COUNT; i++)
        {
            printf("  %-16s %s\n", api_commands[i].name, api_commands[i].help);
        }

        return;
    }

    // ищем команду в таблице
    for (uint i = 0; i < API_COMMAND_COUNT; i++)
    {
        if (strcmp(command->name, api_commands[i].name) == 0)
        {
            api_commands[i].callback(command);
            return;
        }
    }

    printf("error: unknown command '%s', try help\n", command->name);
}
