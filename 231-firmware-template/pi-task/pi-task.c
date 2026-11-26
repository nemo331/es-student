#include "systime/systime.h"
#include "pi-task.h"
#include <stdio.h>

// прикидка переехала в pi-task.h: её использует и api

// запрос расчёта: задача его увидит и выполнит в очередном обороте цикла
static bool pending = false;
static uint32_t terms = PI_TASK_DEFAULT_TERMS;

// сумма ряда печатается в отладчик и остаётся доступной после расчёта
volatile double pi_result;

void pi_task_init(void)
{
    pending = false;
    terms = PI_TASK_DEFAULT_TERMS;
}

void pi_task_request(uint32_t new_terms)
{
    terms = new_terms;
    pending = true;
}

// ряд Лейбница: 4*(1 - 1/3 + 1/5 - ...)
static double calc_pi(uint32_t count)
{
    // сумма ряда и знак очередного члена, оба double
    double sum = 0.0;
    double sign = 1.0;

    for (uint32_t k = 0; k < count; k++)
    {
        sum += sign / (2 * k + 1);
        sign = -sign;
    }

    // сумма ряда, умноженная на 4
    return sum * 4.0;
}

void pi_task_handle(void)
{
    // запроса нет — и дела нет
    if (!pending)
    {
        return;
    }

    pending = false;

    uint64_t start_us = systime_us();
    pi_result = calc_pi(terms);
    uint64_t spent_us = systime_us() - start_us;

    printf("pi: %.9f\n", pi_result);
    printf("time: %llu ms\n", (unsigned long long)(spent_us / 1000));
}
