#pragma once

#include "stdio-text-protocol/stdio-text-protocol.h"

// находит команду в таблице и выполняет её обработчик
void api_handle(const command_t *command);
