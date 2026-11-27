#include "telemetry-task/telemetry-task.h"

#include <stddef.h>

#include "systime/systime.h"
#include "platform/platform.h"

// минимальный период: быстрее журнал не вывозит
#define TELEMETRY_MIN_PERIOD_MS 10

static telemetry_task_callback_t on_record = NULL;
static telemetry_state_t state = TELEMETRY_STATE_IDLE;
static uint32_t period_ms = 0;
static uint64_t last_us = 0;

void telemetry_task_init(telemetry_task_callback_t callback)
{
    on_record = callback;
    state = TELEMETRY_STATE_IDLE;
    period_ms = 0;
    last_us = 0;
}

void telemetry_task_handle(void)
{
    if (state != TELEMETRY_STATE_RUN)
    {
        return;
    }

    // следующий момент отсчитывается от прошлого, а не от «сейчас»,
    // поэтому опоздания не накапливаются
    if (!systime_period_elapsed(&last_us, (uint64_t)period_ms * 1000))
    {
        return;
    }

    // запись: время отсчёта, напряжение и температура у platform
    telemetry_record_t record;
    record.time_us = systime_us();
    record.voltage_v = platform_voltage_read();
    record.temp_c = platform_temp_read();

    // печатает тот, кому отдали запись
    if (on_record != NULL)
    {
        on_record(&record);
    }
}

bool telemetry_task_start(uint32_t new_period_ms)
{
    // период меньше 10 мс не принять
    if (new_period_ms < TELEMETRY_MIN_PERIOD_MS)
    {
        return false;
    }

    period_ms = new_period_ms;

    // первая запись — на ближайшей итерации
    last_us = systime_us() - (uint64_t)period_ms * 1000;

    state = TELEMETRY_STATE_RUN;

    return true;
}

void telemetry_task_stop(void)
{
    state = TELEMETRY_STATE_IDLE;
}

telemetry_state_t telemetry_task_get_state(void)
{
    return state;
}

uint32_t telemetry_task_get_period_ms(void)
{
    return period_ms;
}
