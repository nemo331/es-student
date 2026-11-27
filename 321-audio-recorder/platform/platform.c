#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"

// выводы прибора: их знает только этот файл
#define PLATFORM_LED_PIN    PICO_DEFAULT_LED_PIN
#define PLATFORM_BUTTON_PIN 15

// вывод, через резистор заряжающий конденсатор RC-цепи
#define CHARGE_PIN 22

// вход АЦП: вывод GP26, канал 0
#define ADC_PIN 26
#define ADC_CHANNEL 0

// встроенный датчик температуры — канал 4
#define ADC_TEMP_CHANNEL 4

// опорное напряжение АЦП на плате Pico и число уровней 12-разрядного кода
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

// при 27 °C напряжение на датчике около 0,706 В и падает
// на 1,721 мВ на каждый градус (RP2040 Datasheet, 4.9.5)
#define TEMP_SENSOR_27C_V 0.706f
#define TEMP_SENSOR_SLOPE_V 0.001721f

// состояние мигания по будильнику; переменные, общие с обработчиком
// прерывания, объявлены с volatile: компилятор иначе вправе держать
// их копии в регистрах
static repeating_timer_t blink_timer;
static bool blink_running;
static volatile bool blink_on;
static volatile uint32_t blink_toggles;

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

    // заряд RC-цепи: GP22 на выход, на выводе ноль
    gpio_init(CHARGE_PIN);
    gpio_set_dir(CHARGE_PIN, GPIO_OUT);
    gpio_put(CHARGE_PIN, false);

    // АЦП: сброс и включение модуля, затем отдача вывода аналоговому
    // входу — она отключает у вывода цифровую часть и подтяжки,
    // которые иначе исказили бы напряжение
    adc_init();
    adc_gpio_init(ADC_PIN);

    // питание датчика температуры включают отдельным битом TS_EN
    adc_set_temp_sensor_enabled(true);
}

void platform_led_set(bool on)
{
    gpio_put(PLATFORM_LED_PIN, on);
}

void platform_charge_set(bool on)
{
    // 3,3 В на GP22 или ноль
    gpio_put(CHARGE_PIN, on);
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

// температура кристалла по встроенному датчику, градусы Цельсия
float platform_temp_read(void)
{
    // каждая функция выбирает свой канал перед отсчётом,
    // поэтому get_voltage и get_temp можно вызывать в любом порядке
    adc_select_input(ADC_TEMP_CHANNEL);

    float voltage_v = adc_read() * ADC_VREF_V / ADC_LEVELS;

    // T = 27 − (U − 0,706) / 0,001721
    return 27.0f - (voltage_v - TEMP_SENSOR_27C_V) / TEMP_SENSOR_SLOPE_V;
}

// обработчик будильника индикатора: его вызывает прерывание таймера
// посреди чужой итерации суперцикла, поэтому в нём три строки и ни
// одного вывода в порт
static bool on_blink(repeating_timer_t *timer)
{
    (void)timer;

    blink_on = !blink_on;
    gpio_put(PLATFORM_LED_PIN, blink_on);
    blink_toggles++;

    // true — вызвать снова через тот же период
    return true;
}

bool platform_blink_start(uint32_t period_ms)
{
    if (period_ms < 10 || period_ms > 60000)
    {
        return false;
    }

    platform_blink_stop();
    blink_toggles = 0;

    // минус перед периодом — отсчитывать от начала прошлого вызова,
    // вызовы идут по ровной сетке, опоздания не накапливаются
    blink_running = add_repeating_timer_ms(-(int32_t)period_ms, on_blink, NULL, &blink_timer);
    return blink_running;
}

uint32_t platform_blink_stop(void)
{
    if (blink_running)
    {
        cancel_repeating_timer(&blink_timer);
        blink_running = false;
    }

    blink_on = false;
    gpio_put(PLATFORM_LED_PIN, 0);
    return blink_toggles;
}
