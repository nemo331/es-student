#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "platform/platform.h"

// функция, которую задача вызывает на каждое нажатие кнопки
typedef void (*button_task_callback_t)(void);

// передаёт задаче кнопку и её действие; о том, что делает нажатие,
// задача не знает — связь задаёт main.c
void button_task_init(platform_button_t button, button_task_callback_t on_press);

// обходит все кнопки платы
void button_task_handle(void);

bool button_task_is_pressed(platform_button_t button);
uint32_t button_task_get_press_count(platform_button_t button);
