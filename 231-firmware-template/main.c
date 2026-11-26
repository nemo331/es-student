#include "pico/stdlib.h"
#include "led-task.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"
#include "button-task.h"

int main()
{
    // инициализируем стандартный ввод-вывод
    stdio_init_all();

    // задача светодиода: настраивает вывод, период 1000 мс, состояние «мигает»
    led_task_init();

    // задача кнопки: вывод на вход с подтяжкой, машина дребезга
    button_task_init();

    // запоминаем начальное показание часов для профилирования
    profiling_init();

    stdio_text_protocol_init();

    while (1)
    {
        // учитываем прошлую итерацию
        profiling_iteration();

        // задача светодиода
        led_task_handle();

        // задача кнопки
        button_task_handle();

        // принятая строка становится командой, команда — действием прибора
        const command_t *command = stdio_text_protocol_handle();

        if (command != NULL)
        {
            api_handle(command);
        }
    }
}
