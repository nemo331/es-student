#include "led-task/led-task.h"

#include "systime/systime.h"
#include "platform/platform.h"

static led_state_t led_state = LED_STATE_BLINK;
static uint32_t period_ms = 1000;
static uint64_t last_toggle_us = 0;
static bool led_on = false;

void led_task_init(void)
{
    period_ms = 1000;

    // первое переключение сразу: светодиод загорается при старте
    led_task_set_state(LED_STATE_BLINK);
}

void led_task_set_state(led_state_t state)
{
    led_state = state;

    // действие при входе в состояние
    switch (led_state)
    {
    case LED_STATE_OFF:
        led_on = false;
        platform_led_set(led_on);
        break;

    case LED_STATE_ON:
        led_on = true;
        platform_led_set(led_on);
        break;

    case LED_STATE_BLINK:
        // первое переключение сразу: светодиод загорается, отсчёт заново
        led_on = true;
        platform_led_set(led_on);
        last_toggle_us = systime_us();
        break;
    }
}

led_state_t led_task_get_state(void)
{
    return led_state;
}

bool led_task_set_period_ms(uint32_t new_period_ms)
{
    // нулевой период не имеет смысла — отказываем
    if (new_period_ms == 0)
    {
        return false;
    }

    period_ms = new_period_ms;
    return true;
}

uint32_t led_task_get_period_ms(void)
{
    return period_ms;
}

void led_task_handle(void)
{
    switch (led_state)
    {
    // «выключен» и «горит» — ничего не делаем, светодиод установлен при входе
    case LED_STATE_OFF:
    case LED_STATE_ON:
        break;

    // «мигает» — каждые полпериода переключаем светодиод
    case LED_STATE_BLINK:
        if (systime_period_elapsed(&last_toggle_us, (uint64_t)period_ms * 1000 / 2))
        {
            led_on = !led_on;
            platform_led_set(led_on);
        }
        break;
    }
}

// переводит светодиод в следующее состояние по кругу, начиная с текущего
void led_task_next_state(void)
{
    switch (led_state)
    {
    case LED_STATE_OFF:
        led_task_set_state(LED_STATE_ON);
        break;

    case LED_STATE_ON:
        led_task_set_state(LED_STATE_BLINK);
        break;

    case LED_STATE_BLINK:
        led_task_set_state(LED_STATE_OFF);
        break;
    }
}
