#pragma once

#include <stdint.h>

#include "stdio-text-protocol/stdio-text-protocol.h"

// тип обработчика: команда приходит целиком
typedef void (*api_callback_t)(const command_t *command);

// строка таблицы: имя, справка и обработчик
typedef struct
{
    const char *name;
    const char *help;
    api_callback_t callback;
} api_command_t;

// таблица определена в api-callbacks.c рядом с обработчиками;
// константная, поэтому открыть её через extern можно
extern const api_command_t api_commands[];
extern const uint32_t api_command_count;
