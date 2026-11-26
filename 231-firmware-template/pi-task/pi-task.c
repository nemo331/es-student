#include "pico/stdlib.h"

#include "pi-task.h"

// цена члена ряда — около 585 тактов, то есть 4,7 мкс при 125 МГц.
// Порция должна длиться не дольше 0,5 мс = 62 500 тактов, это около ста членов:
// 100 × 585 = 58 500 тактов ≈ 468 мкс
#define PI_TERMS_PER_CALL 100

// члены ряда бывают до 4 294 967 295 — по Ф5.1 всё в uint32_t
static pi_state_t state = PI_STATE_NOT_STARTED;
static uint32_t k = 0;          // номер следующего члена
static uint32_t terms = 0;      // сколько членов всего
static double sum = 0.0;
static double sign = 1.0;
static uint64_t start_us = 0;
static uint64_t end_us = 0;

void pi_task_init(void)
{
    state = PI_STATE_NOT_STARTED;
}

// запускает расчёт; пока идёт расчёт — отказывает
bool pi_task_start(uint32_t new_terms)
{
    if (state == PI_STATE_RUNNING)
    {
        return false;
    }

    terms = new_terms;
    k = 0;
    sum = 0.0;
    sign = 1.0;
    start_us = time_us_64();
    state = PI_STATE_RUNNING;

    return true;
}

pi_state_t pi_task_get_state(void)
{
    return state;
}

// отдаёт результат и время расчёта; до завершения — отказывает
bool pi_task_get_result(double *value, uint32_t *elapsed_ms)
{
    if (state != PI_STATE_DONE)
    {
        return false;
    }

    *value = sum * 4.0;
    *elapsed_ms = (uint32_t)((end_us - start_us) / 1000);

    return true;
}

void pi_task_handle(void)
{
    // не считаем — ничего не делаем
    if (state != PI_STATE_RUNNING)
    {
        return;
    }

    // конец порции: сложение pi_k + PI_TERMS_PER_CALL возле 4 294 967 295
    // не помещается в 32 бита и становится маленьким числом.
    // Если конец оказался меньше начала — сумма переполнилась,
    // и концом порции становится последний член
    uint32_t end = k + PI_TERMS_PER_CALL;

    if (end < k)
    {
        end = terms;
    }

    if (end > terms)
    {
        end = terms;
    }

    for (; k < end; k++)
    {
        sum += sign / (2.0 * k + 1.0);
        sign = -sign;
    }

    // все члены посчитаны — фиксируем время и состояние
    if (k >= terms)
    {
        end_us = time_us_64();
        state = PI_STATE_DONE;
    }
}
