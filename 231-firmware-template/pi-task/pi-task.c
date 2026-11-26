#include <stdio.h>
#include "systime/systime.h"
#include "pi-task.h"

// прикидка переехала в pi-task.h: её используют и api

// состояние расчёта: не запускался, считает, готово
static pi_task_state_t state = PI_TASK_NOT_STARTED;

// параметры расчёта
static uint32_t total_terms = PI_TASK_DEFAULT_TERMS;
static uint32_t done_terms = 0;
static double sum = 0.0;
static double sign = 1.0;
static uint64_t start_us = 0;
static uint64_t spent_us = 0;

// сумма ряда печатается в отладчик и остаётся доступной после расчёта
volatile double pi_result;

void pi_task_init(void)
{
    state = PI_TASK_NOT_STARTED;
    total_terms = PI_TASK_DEFAULT_TERMS;
    done_terms = 0;
    sum = 0.0;
    sign = 1.0;
    start_us = 0;
    spent_us = 0;
}

bool pi_task_start(uint32_t new_terms)
{
    // занята — запрос не принимаем
    if (state == PI_TASK_CALCULATING)
    {
        return false;
    }

    total_terms = new_terms;
    done_terms = 0;
    sum = 0.0;
    sign = 1.0;
    start_us = systime_us();
    state = PI_TASK_CALCULATING;

    return true;
}

// ряд Лейбница считают порциями по PI_TERMS_PER_CALL за оборот цикла,
// чтобы суперцикл оставался отзывчивым между порциями
void pi_task_handle(void)
{
    if (state != PI_TASK_CALCULATING)
    {
        return;
    }

    uint32_t until = done_terms + PI_TERMS_PER_CALL;

    if (until > total_terms)
    {
        until = total_terms;
    }

    for (; done_terms < until; done_terms++)
    {
        sum += sign / (2 * done_terms + 1);
        sign = -sign;
    }

    // порция дозрела до конца ряда — расчёт готов
    if (done_terms >= total_terms)
    {
        pi_result = sum * 4.0;
        spent_us = systime_us() - start_us;
        state = PI_TASK_DONE;
    }
}

pi_task_state_t pi_task_get_state(void)
{
    return state;
}

bool pi_task_get_result(double *result, uint64_t *elapsed_us)
{
    if (state != PI_TASK_DONE || result == NULL || elapsed_us == NULL)
    {
        return false;
    }

    *result = pi_result;
    *elapsed_us = spent_us;

    return true;
}
