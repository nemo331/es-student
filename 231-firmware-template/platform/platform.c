#include "pico/stdlib.h"
#include "platform/platform.h"

// выводы прибора: их знает только этот файл
#define PLATFORM_LED_PIN    PICO_DEFAULT_LED_PIN
#define PLATFORM_BUTTON_PIN 15

void platform_init(void)
{
    // инициализируем стандартный ввод-вывод
    stdio_init_all();

    // светодиод: направление — выход, уровень — ноль
    gpio_init(PLATFORM_LED_PIN);
    gpio_set_dir(PLATFORM_LED_PIN, GPIO_OUT);
    gpio_put(PLATFORM_LED_PIN, false);

    // кнопка: замыкание на землю, поэтому подтяжка к питанию
    gpio_init(PLATFORM_BUTTON_PIN);
    gpio_set_dir(PLATFORM_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(PLATFORM_BUTTON_PIN);
}

void platform_led_set(bool on)
{
    gpio_put(PLATFORM_LED_PIN, on);
}

bool platform_button_read(void)
{
    // нажатая кнопка замыкает вывод на землю и даёт ноль
    return gpio_get(PLATFORM_BUTTON_PIN) == 0;
}
