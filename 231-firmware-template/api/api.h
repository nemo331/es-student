#pragma once

#include "stdio-text-protocol.h"

// принимает разобранную команду и выполняет её по таблице
void api_handle(const command_t *command);
