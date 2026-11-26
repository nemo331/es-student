#include "profiling.h"
#include "pico/stdlib.h"

// секунodмеры лежат массивом, адресуются номером — как выводы GPIO номером.
// Состояние модуля — в переменных файла со static: второй набор секундомеров
// прибору не нужен
static const char *names[PROFILING_STOPWATCH_COUNT];
static uint32_t start_us[PROFILING_STOPWATCH_COUNT];
static uint32_t sum_us[PROFILING_STOPWATCH_COUNT];
static uint32_t max_us[PROFILING_STOPWATCH_COUNT];
static uint32_t counts[PROFILING_STOPWATCH_COUNT];
static bool inited[PROFILING_STOPWATCH_COUNT];

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    if (id >= PROFILING_STOPWATCH_COUNT)
    {
        return;
    }

    names[id] = name;
    start_us[id] = 0;
    sum_us[id] = 0;
    max_us[id] = 0;
    counts[id] = 0;
    inited[id] = true;
}

void profiling_start(uint32_t id)
{
    if (id >= PROFILING_STOPWATCH_COUNT || !inited[id])
    {
        return;
    }

    start_us[id] = time_us_32();
}

void profiling_stop(uint32_t id)
{
    if (id >= PROFILING_STOPWATCH_COUNT || !inited[id])
    {
        return;
    }

    // разность переживёт переполнение time_us_32() через 71,6 минуты
    uint32_t now_us = time_us_32();
    uint32_t spent_us = now_us - start_us[id];

    sum_us[id] += spent_us;
    counts[id]++;

    if (spent_us > max_us[id])
    {
        max_us[id] = spent_us;
    }
}

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= PROFILING_STOPWATCH_COUNT || !inited[id])
    {
        return false;
    }

    result->name = names[id];
    result->count = counts[id];
    result->max_us = max_us[id];

    // среднее арифметическое: деление и float здесь допустимы —
    // вызов бывает только по команде, а не на каждой итерации
    result->mean_us = counts[id] ? (float)sum_us[id] / (float)counts[id] : 0.0f;

    return true;
}

void profiling_reset(void)
{
    for (uint32_t i = 0; i < PROFILING_STOPWATCH_COUNT; i++)
    {
        sum_us[i] = 0;
        max_us[i] = 0;
        counts[i] = 0;
    }
}
