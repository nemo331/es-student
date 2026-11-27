#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

// выводы прибора: их знает только этот файл
#define PLATFORM_LED_PIN    PICO_DEFAULT_LED_PIN
#define PLATFORM_BUTTON_PIN 15

// вход АЦП: вывод GP26, канал 0
#define ADC_PIN 26
#define ADC_CHANNEL 0

// опорное напряжение АЦП на плате Pico и число уровней 12-разрядного кода
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

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

    // АЦП: сброс и включение модуля, затем отдача вывода аналоговому
    // входу — она отключает у вывода цифровую часть и подтяжки,
    // которые иначе исказили бы напряжение
    adc_init();
    adc_gpio_init(ADC_PIN);
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

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);

    // код 0..4095 умножаем на опорное напряжение и делим на число уровней
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}
