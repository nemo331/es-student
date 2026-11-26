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

// паспорт печатает обработчик, модуль только отдаёт данные
static void cmd_info(const command_t *command)
{
    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", DEVICE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", DEVICE_PROJECT);
    printf("repo: %s\n", DEVICE_REPO);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
    printf("pico-sdk: %s\n", info.sdk);
}

static void cmd_uptime(const command_t *command)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

static void cmd_pi_start(const command_t *command)
{
    uint32_t terms = 1000000;

    if (command->argc > 1)
    {
        printf("error: usage pi_start [terms]\n");
        return;
    }

    if (command->argc == 1)
    {
        if (!parse_u32(command->argv[0], &terms))
        {
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

// все секундомеры разом; строка каждого — как в приложении Б
static void cmd_profiling(const command_t *command)
{
    for (uint32_t i = 0; i < PROFILING_STOPWATCH_COUNT; i++)
    {
        profiling_result_t result;

        if (profiling_get(i, &result))
        {
            printf("%-9s count %-9u mean %.2f us  max %u us\n",
                   result.name,
                   (unsigned)result.count,
                   result.mean_us,
                   (unsigned)result.max_us);
        }
    }
}

static void cmd_profiling_reset(const command_t *command)
{
    profiling_reset();
    printf("profiling reset\n");
}

// ---- команды светодиода ----

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
    { "info",             "print device passport",                              cmd_info },
    { "uptime",           "print milliseconds since reset",                     cmd_uptime },
    { "pi_start",         "start pi computation; optional arg: terms",          cmd_pi_start },
    { "pi",               "report pi computation result or state",              cmd_pi },
    { "profiling",        "print all stopwatches: count, mean, max",            cmd_profiling },
    { "profiling_reset",  "reset all stopwatches",                              cmd_profiling_reset },
    { "led_enable",       "turn the led on",                                    cmd_led_enable },
    { "led_disable",      "turn the led off",                                   cmd_led_disable },
    { "led_blink",        "blink the led",                                      cmd_led_blink },
    { "led_period",       "set blink period in ms; arg: period_ms > 0",         cmd_led_period },
    { "button",           "report button state and press count",                cmd_button },
};

#define API_COMMAND_COUNT (sizeof(api_commands) / sizeof(api_commands[0]))

void api_handle(const command_t *command)
{
    if (command->truncated)
    {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }

    if (strcmp(command->name, "help") == 0)
    {
        printf("help: print this list\n");

        for (uint i = 0; i < API_COMMAND_COUNT; i++)
        {
            printf("  %-16s %s\n", api_commands[i].name, api_commands[i].help);
        }

        return;
    }

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
