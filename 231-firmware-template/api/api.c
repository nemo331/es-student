#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pico/stdlib.h"
#include "api.h"
#include "device.h"
#include "profiling.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"

// тип обработчика: команда приходит целиком
typedef void (*api_callback_t)(const command_t *command);

// строка таблицы: имя, справка и обработчик
typedef struct
{
    const char *name;
    const char *help;
    api_callback_t callback;
} api_command_t;

// ---- разбор числа ----

// разбирает десятичное число целиком: без хвоста, без переполнения, без пустой строки
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
const uint32_t PI_DEFAULT_TERMS = 1000000;

static void cmd_pi_start(const command_t *command)
{
    // число членов ряда — необязательный аргумент
    uint32_t terms = (uint32_t)PI_DEFAULT_TERMS;

    if (command->argc > 1)
    {
        // лишние аргументы
        printf("error: usage pi_start [terms]\n");
        return;
    }

    if (command->argc == 1)
    {
        if (!parse_u32(command->argv[0], &terms))
        {
            // неверная запись числа
            printf("error: usage pi_start [terms]\n");
            return;
        }

        if (terms == 0)
        {
            printf("error: terms must be greater than zero\n");
            return;
        }
    }

    if (!pi_task_start(terms))
    {
        // расчёт уже идёт
        printf("error: pi is busy\n");
        return;
    }

    printf("pi: started, %u terms\n", (unsigned)terms);
}

static void cmd_pi(const command_t *command)
{
    switch (pi_task_get_state())
    {
    case PI_STATE_NOT_STARTED:
        printf("pi: not started\n");
        break;

    case PI_STATE_RUNNING:
        printf("pi: running\n");
        break;

    case PI_STATE_DONE:
    {
        double value = 0.0;
        uint32_t elapsed_ms = 0;

        if (pi_task_get_result(&value, &elapsed_ms))
        {
            printf("pi: %.9f in %u ms\n", value, (unsigned)elapsed_ms);
        }

        break;
    }
    }
}

static void cmd_main_time_exec(const command_t *command)
{
    // среднее и максимум длительности итерации суперцикла
    printf("iteration avg %.2f us, max %u us\n",
           profiling_avg_us(), (unsigned)profiling_max_us());
}

static void cmd_main_time_reset(const command_t *command)
{
    profiling_reset_max();
    printf("max reset\n");
}

// ---- команды светодиода ----

// ответ всех команд светодиода: led: blink, period 1000 ms
static void print_led(void)
{
    const char *name;

    switch (led_task_get_state())
    {
    case LED_STATE_OFF:
        name = "off";
        break;

    case LED_STATE_ON:
        name = "on";
        break;

    case LED_STATE_BLINK:
        name = "blink";
        break;

    default:
        name = "?";
        break;
    }

    printf("led: %s, period %u ms\n", name, (unsigned)led_task_get_period_ms());
}

static void cmd_led_enable(const command_t *command)
{
    led_task_set_state(LED_STATE_ON);
    print_led();
}

static void cmd_led_disable(const command_t *command)
{
    led_task_set_state(LED_STATE_OFF);
    print_led();
}

static void cmd_led_blink(const command_t *command)
{
    led_task_set_state(LED_STATE_BLINK);
    print_led();
}

static void cmd_led_period(const command_t *command)
{
    uint32_t period_ms;

    if (command->argc != 1 || !parse_u32(command->argv[0], &period_ms))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    if (period_ms == 0)
    {
        printf("error: period must be greater than zero\n");
        return;
    }

    // период применяется и в состоянии «горит»: начнёт мигать — уже новым периодом
    led_task_set_period_ms(period_ms);
    print_led();
}

// ---- команда кнопки ----

static void cmd_button(const command_t *command)
{
    printf("button: %s, presses %u\n",
           button_task_is_pressed() ? "pressed" : "released",
           (unsigned)button_task_get_press_count());
}

// ---- таблица команд ----

static const api_command_t api_commands[] = {
    { "info",            "print device passport",                              cmd_info },
    { "uptime",          "print milliseconds since reset",                     cmd_uptime },
    { "pi_start",        "start pi computation; optional arg: terms",          cmd_pi_start },
    { "pi",              "report pi computation result or state",              cmd_pi },
    { "main_time_exec",  "print superloop iteration average and maximum time", cmd_main_time_exec },
    { "main_time_reset", "reset iteration maximum",                            cmd_main_time_reset },
    { "led_enable",      "turn the led on",                                    cmd_led_enable },
    { "led_disable",     "turn the led off",                                   cmd_led_disable },
    { "led_blink",       "blink the led",                                      cmd_led_blink },
    { "led_period",      "set blink period in ms; arg: period_ms > 0",         cmd_led_period },
    { "button",          "report button state and press count",                cmd_button },
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
