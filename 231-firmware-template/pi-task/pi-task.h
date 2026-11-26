#pragma once

#include <stdint.h>

// прикидка: за член ряда 4 операции с double, 175 + 110 + 190 + 110 = 585 тактов;
// 1 000 000 членов по 585 тактов при 125 МГц — около 4,7 с
#define PI_TASK_DEFAULT_TERMS 1000000

void pi_task_init(void);

// запрашивает расчёт с числом членов ряда; расчёт выполнит pi_task_handle
void pi_task_request(uint32_t terms);

void pi_task_handle(void);
