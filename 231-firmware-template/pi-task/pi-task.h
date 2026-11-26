#pragma once

#include <stdbool.h>
#include <stdint.h>

// прикидка: за член ряда 4 операции с double, 175 + 110 + 190 + 110 = 585 тактов;
// 1 000 000 членов по 585 тактов при 125 МГц — около 4,7 с
#define PI_TASK_DEFAULT_TERMS 1000000

// членов ряда за один вызов pi_task_handle: около 0,5 с при 125 МГц,
// суперцикл остаётся отзывчивым между порциями
#define PI_TERMS_PER_CALL 100000

// состояния задачи: не запускался, считает, готово
typedef enum
{
    PI_TASK_NOT_STARTED,
    PI_TASK_CALCULATING,
    PI_TASK_DONE,
} pi_task_state_t;

void pi_task_init(void);

// запрашивает расчёт; если задача уже считает, отказывает и возвращает false
bool pi_task_start(uint32_t terms);

// считает очередную порцию ряда
void pi_task_handle(void);

pi_task_state_t pi_task_get_state(void);

// готовый результат и время расчёта; false — расчёт ещё не готов
bool pi_task_get_result(double *result, uint64_t *spent_us);
