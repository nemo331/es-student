#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "led.h"

const uint LED_PIN = 25;

static bool led_state = false;

void led_init(void)
{
    // Настраиваем пин как выход
    gpio_set_function(LED_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(LED_PIN, true);  // true = выход

    // Устанавливаем начальное состояние (выключено)
    led_set(false);
}

void led_set(bool on)
{
    led_state = on;
    gpio_put(LED_PIN, led_state ? 1 : 0);
}

void led_toggle(void)
{
    led_set(!led_state);
}

bool led_is_on(void)
{
    return led_state;
}
