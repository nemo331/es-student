#include "pico/stdlib.h"
#include "systime/systime.h"
#include "led-task.h"

// выводом светодиода владеет задача
static const uint LED_TASK_PIN = PICO_DEFAULT_LED_PIN;

static led_state_t led_state = LED_STATE_BLINK;
static uint32_t period_ms = 1000;
static uint64_t last_toggle_us = 0;

void led_task_init(void)
{
    // настраиваем вывод светодиода: направление — выход, уровень — ноль
    gpio_init(LED_TASK_PIN);
    gpio_set_dir(LED_TASK_PIN, GPIO_OUT);
    gpio_put(LED_TASK_PIN, false);

    period_ms = 1000;
    last_toggle_us = systime_us();

    // первое переключение сразу: светодиод загорается при старте
    led_state = LED_STATE_BLINK;
    gpio_put(LED_TASK_PIN, true);
}

void led_task_set_state(led_state_t state)
{
    led_state = state;

    // действие при входе в состояние
    switch (led_state)
    {
    case LED_STATE_OFF:
        gpio_put(LED_TASK_PIN, false);
        break;

    case LED_STATE_ON:
        gpio_put(LED_TASK_PIN, true);
        break;

    case LED_STATE_BLINK:
        // первое переключение сразу: светодиод загорается, отсчёт заново
        gpio_put(LED_TASK_PIN, true);
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
            gpio_xor_mask(1u << LED_TASK_PIN);
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
