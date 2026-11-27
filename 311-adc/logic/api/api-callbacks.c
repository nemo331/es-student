#include "api/api-commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "device/device.h"
#include "profiling/profiling.h"
#include "systime/systime.h"

#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "firmware.h"
#include "platform/platform.h"

// разбирает десятичное число: только цифры, от 0 до 4 294 967 295.
// Знак, посторонние символы и переполнение — ошибка записи
static bool parse_u32(const char *text, uint32_t *value)
{
    if (text == NULL || *text == '\0')
    {
        return false;
    }

    // только цифры: strtoul сам принимает знак и пробелы, а этого нельзя
    for (const char *p = text; *p != '\0'; p++)
    {
        if (*p < '0' || *p > '9')
        {
            return false;
        }
    }

    errno = 0;
    char *end = NULL;
    unsigned long number = strtoul(text, &end, 10);

    // переполнение: число не влезло в unsigned long (32 бита на ARM)
    if (errno == ERANGE)
    {
        return false;
    }

    *value = (uint32_t)number;
    return true;
}

// ---- обработчики ----

static void cmd_info(const command_t *command)
{
    (void)command;

    device_info_t info;
    device_get_info(&info);

    // восемь строк в порядке таблицы приложения Б
    printf("name: %s\n", FIRMWARE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", FIRMWARE_PROJECT);
    printf("repo: %s\n", FIRMWARE_REPO);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
    printf("pico-sdk: %s\n", info.sdk);
}

static void cmd_uptime(const command_t *command)
{
    (void)command;
    printf("uptime: %llu ms\n", (unsigned long long)systime_ms());
}

// ---- команды светодиода: единый формат ответа ----

// имя состояния для ответа
static const char *led_state_name(led_state_t state)
{
    switch (state)
    {
    case LED_STATE_OFF:
        return "off";

    case LED_STATE_ON:
        return "on";

    case LED_STATE_BLINK:
        return "blink";
    }

    return "?";
}

// печатает состояние светодиода: led: <состояние>, period <N> ms
static void print_led(void)
{
    printf("led: %s, period %u ms\n",
           led_state_name(led_task_get_state()),
           (unsigned)led_task_get_period_ms());
}

static void cmd_led_enable(const command_t *command)
{
    (void)command;

    led_task_set_state(LED_STATE_ON);
    print_led();
}

static void cmd_led_disable(const command_t *command)
{
    (void)command;

    led_task_set_state(LED_STATE_OFF);
    print_led();
}

static void cmd_led_blink(const command_t *command)
{
    (void)command;

    led_task_set_state(LED_STATE_BLINK);
    print_led();
}

static void cmd_led_period(const command_t *command)
{
    // период в миллисекундах — обязательный аргумент
    if (command->argc != 1)
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    uint32_t period;

    if (!parse_u32(command->argv[0], &period))
    {
        // знак, посторонние символы или переполнение — ошибка записи
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    if (period == 0)
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }

    led_task_set_period_ms(period);
    print_led();
}

// ---- команда кнопки ----

static void cmd_button(const command_t *command)
{
    (void)command;
    printf("button: %s, presses %u\n",
           button_task_is_pressed() ? "pressed" : "released",
           (unsigned)button_task_get_press_count());
}

// ---- команды расчёта π ----

static void cmd_pi_start(const command_t *command)
{
    // число членов ряда — необязательный аргумент; argc считает только
    // аргументы, имя команды в них не входит
    uint32_t terms = PI_TASK_DEFAULT_TERMS;

    if (command->argc > 1)
    {
        printf("error: usage pi_start [<terms>]\n");
        return;
    }

    if (command->argc == 1)
    {
        if (!parse_u32(command->argv[0], &terms))
        {
            printf("error: usage pi_start [<terms>]\n");
            return;
        }

        if (terms == 0)
        {
            printf("error: terms must be greater than 0\n");
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
    (void)command;

    switch (pi_task_get_state())
    {
    case PI_TASK_NOT_STARTED:
        printf("pi: not started\n");
        break;

    case PI_TASK_CALCULATING:
        printf("pi: running\n");
        break;

    case PI_TASK_DONE:
    {
        double result;
        uint64_t elapsed_us;

        if (pi_task_get_result(&result, &elapsed_us))
        {
            printf("pi: %.9f in %llu ms\n",
                   result,
                   (unsigned long long)(elapsed_us / 1000));
        }
        break;
    }
    }
}

// ---- команды профилирования ----

static void cmd_profiling(const command_t *command)
{
    (void)command;

#if PROFILING_ENABLED
    for (uint32_t i = 0; i < PROFILING_STOPWATCH_COUNT; i++)
    {
        profiling_result_t result;

        if (profiling_get(i, &result))
        {
            // формат из приложения Б: имя и число прижаты влево на 10 позиций,
            // перед max два пробела
            printf("%-10s count %-10u mean %.2f us  max %u us\n",
                   result.name,
                   (unsigned)result.count,
                   result.mean_us,
                   (unsigned)result.max_us);
        }
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}

static void cmd_profiling_reset(const command_t *command)
{
    (void)command;

#if PROFILING_ENABLED
    profiling_reset();
    printf("profiling reset\n");
#else
    printf("profiling is disabled in this build\n");
#endif
}

// напряжение на GP26 у platform, ответ voltage: <напряжение> V
static void cmd_get_voltage(const command_t *command)
{
    (void)command;

    printf("voltage: %.3f V\n", platform_voltage_read());
}


// ---- таблица команд ----

static void cmd_help(const command_t *command)
{
    // перечень печатает механизм в api.c, эта запись только для перечня
    (void)command;
}

// справки help и info взяты из примера приложения Б дословно;
// остальные — перевод описаний из перечня команд
const api_command_t api_commands[] = {
    { "help",            "list of commands",               cmd_help },
    { "info",            "device passport",                cmd_info },
    { "uptime",          "time since reset",               cmd_uptime },
    { "led_enable",      "set led to on",                  cmd_led_enable },
    { "led_disable",     "set led to off",                 cmd_led_disable },
    { "led_blink",       "set led to blink",               cmd_led_blink },
    { "led_period",      "set blink period in ms",         cmd_led_period },
    { "button",          "button state and press count",   cmd_button },
    { "pi_start",        "start pi calculation",           cmd_pi_start },
    { "pi",              "pi result or calculation state", cmd_pi },
    { "profiling",       "stopwatch results",              cmd_profiling },
    { "profiling_reset", "reset stopwatch results",        cmd_profiling_reset },
    { "get_voltage",     "voltage on GP26, ADC input 0",     cmd_get_voltage },

};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);
