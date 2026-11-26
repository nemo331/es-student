#include "platform/platform.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "api/api.h"
#include "profiling/profiling.h"
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "logging/log.h"
#include "firmware.h"

// номера секундомеров выбирает main.c; имена — из приложения Б
typedef enum {
    STOPWATCH_LOOP,
    STOPWATCH_COMMAND,
    STOPWATCH_LED,
    STOPWATCH_BUTTON,
    STOPWATCH_PI,
} stopwatch_t;

int main()
{
    // платформа: стандартный ввод-вывод и выводы
    platform_init();

    // задача светодиода: период 1000 мс, состояние «мигает»
    led_task_init();

    // задача расчёта π
    pi_task_init();

    // протокол
    stdio_text_protocol_init();

    // заводим секундомеры
    profiling_stopwatch_init(STOPWATCH_LOOP, "loop");
    profiling_stopwatch_init(STOPWATCH_COMMAND, "command");
    profiling_stopwatch_init(STOPWATCH_LED, "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON, "button");
    profiling_stopwatch_init(STOPWATCH_PI, "pi");

    // имя и версию прошивки выводит main.c; строку в терминале
    // скорее всего не видно — порт ещё не открыт
    LOG_INF("%s %s\n", FIRMWARE_NAME, FIRMWARE_VERSION);

    // нажатие переводит светодиод в следующее состояние:
    // связь задаёт main.c, задача кнопки не знает led-task
    button_task_init(led_task_next_state);

    while (1)
    {
        profiling_start(STOPWATCH_LOOP);

        // приём символа и выполнение команды — между стартом и остановкой
        profiling_start(STOPWATCH_COMMAND);
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
        profiling_stop(STOPWATCH_COMMAND);

        // обработчик светодиода
        profiling_start(STOPWATCH_LED);
        led_task_handle();
        profiling_stop(STOPWATCH_LED);

        // обработчик кнопки
        profiling_start(STOPWATCH_BUTTON);
        button_task_handle();
        profiling_stop(STOPWATCH_BUTTON);

        // обработчик расчёта π
        profiling_start(STOPWATCH_PI);
        pi_task_handle();
        profiling_stop(STOPWATCH_PI);

        profiling_stop(STOPWATCH_LOOP);
    }
}
