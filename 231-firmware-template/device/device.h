

#pragma once

#include <stdint.h>

// константы прошивки — их знает и журнал через main.c
#define DEVICE_NAME "es-led-module"
#define FIRMWARE_VERSION "1.1.0"

#define DEVICE_PROJECT "231-firmware-template"
#define DEVICE_REPO "https://github.com/nemo331/es-student"
#define DEVICE_VERSION  "1.0.0"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif
// серийный номер: 16 шестнадцатеричных цифр и нулевой символ
#define DEVICE_SERIAL_LENGTH 17

// сведения о плате; модуль возвращает их структурой и ничего не печатает
typedef struct
{
    const char *board;        // тип платы
    char serial[DEVICE_SERIAL_LENGTH];
    uint32_t manufacturer;    // производитель кристалла
    uint32_t part;            // изделие
    uint32_t revision;        // ревизия
    const char *sdk;          // версия SDK
} device_info_t;

void device_get_info(device_info_t *info);
