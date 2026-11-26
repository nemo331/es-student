#pragma once

#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>

#define DEVICE_NAME "es-led-module"
#define FIRMWARE_VERSION "1.1.0"

#define DEVICE_PROJECT "231-firmware-template"
#define DEVICE_REPO "https://github.com/nemo331/es-student"
#define DEVICE_VERSION  "1.0.0"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

void device_info(void);

#endif