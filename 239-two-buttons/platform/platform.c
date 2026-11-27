#include "platform/platform.h"
#include "pico/stdlib.h"

// выводы прибора: их знает только этот файл
#define PLATFORM_LED_PIN       PICO_DEFAULT_LED_PIN
#define PLATFORM_BUTTON_1_PIN  15
#define PLATFORM_BUTTON_2_PIN  14

void platform_init(void)
{
    // инициализируем стандартный ввод-вывод
    stdio_init_all();

    // светодиод: направление — выход, уровень — ноль
    gpio_init(PLATFORM_LED_PIN);
    gpio_set_dir(PLATFORM_LED_PIN, GPIO_OUT);
    gpio_put(PLATFORM_LED_PIN, false);

    // кнопки: замыкание на землю, поэтому подтяжка к питанию
    gpio_init(PLATFORM_BUTTON_1_PIN);
    gpio_set_dir(PLATFORM_BUTTON_1_PIN, GPIO_IN);
    gpio_pull_up(PLATFORM_BUTTON_1_PIN);

    gpio_init(PLATFORM_BUTTON_2_PIN);
    gpio_set_dir(PLATFORM_BUTTON_2_PIN, GPIO_IN);
    gpio_pull_up(PLATFORM_BUTTON_2_PIN);
}

void platform_led_set(bool on)
{
    gpio_put(PLATFORM_LED_PIN, on);
}

bool platform_button_read(platform_button_t button)
{
    uint pin;

    switch (button)
    {
    case PLATFORM_BUTTON_1:
        pin = PLATFORM_BUTTON_1_PIN;
        break;

    case PLATFORM_BUTTON_2:
        pin = PLATFORM_BUTTON_2_PIN;
        break;

    default:
        return false;
    }

    // нажатая кнопка замыкает вывод на землю и даёт ноль
    return gpio_get(pin) == 0;
}
