#include "pico/stdlib.h"
#include "led-task.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"
#include "button-task.h"
#include "pi-task.h"

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

    pi_task_init();

    while (1)
    {
        profiling_iteration();

        led_task_handle();

        button_task_handle();

        // задача расчёта: считает порцию членов ряда
        pi_task_handle();

        const command_t *command = stdio_text_protocol_handle();

        if (command != NULL)
        {
            api_handle(command);
        }
    }
}
