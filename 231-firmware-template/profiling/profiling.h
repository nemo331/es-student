#pragma once

#include <stdbool.h>
#include <stdint.h>

// секундомеров в приборе; номера выбирает main.c
#define PROFILING_STOPWATCH_COUNT 5

// сумма замеров хранится в 32 битах: это 2^32 микросекунд,
// один секундомер переполнится только через 71 минуту непрерывного счёта.
// Дешевле 64 бит, а дольше жизни одной сессии итерации хватает

// результат одного секундомера: имя, число замеров, среднее и максимум (мкс)
typedef struct
{
    const char *name;
    uint32_t count;
    float mean_us;
    uint32_t max_us;
} profiling_result_t;

void profiling_stopwatch_init(uint32_t id, const char *name);
void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);
