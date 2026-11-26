#include "pico/stdlib.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"
#include "log.h"
#include "device.h"

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
    // инициализируем стандартный ввод-вывод
    stdio_init_all();

    // задача светодиода: настраивает вывод, период 1000 мс, состояние «мигает»
    led_task_init();

    // задача кнопки: вывод на вход с подтяжкой, машина дребезга
    button_task_init();

    // задача расчёта π
    pi_task_init();

    stdio_text_protocol_init();

    // заводим секундомеры
    profiling_stopwatch_init(STOPWATCH_LOOP, "loop");
    profiling_stopwatch_init(STOPWATCH_COMMAND, "command");
    profiling_stopwatch_init(STOPWATCH_LED, "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON, "button");
    profiling_stopwatch_init(STOPWATCH_PI, "pi");

    // имя и версию прошивки выводит main.c: журнал о device не знает,
    // и эту строку в терминале, скорее всего, не видно — порт ещё не открыт
    LOG_INF("%s %s\n", DEVICE_NAME, FIRMWARE_VERSION);

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
