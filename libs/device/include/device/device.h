#pragma once

#include <stdint.h>

// серийный номер: 16 шестнадцатеричных цифр и нулевой символ
#define DEVICE_SERIAL_LENGTH 17

// сведения о плате; модуль возвращает их структурой и ничего не печатает
typedef struct
{
    const char *board;
    char serial[DEVICE_SERIAL_LENGTH];
    uint32_t manufacturer;
    uint32_t part;
    uint32_t revision;
    const char *sdk;
} device_info_t;

void device_get_info(device_info_t *info);
