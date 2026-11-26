#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "button-task.h"
#include "led-task.h"

// вывод кнопки: замыкание на землю, поэтому подтяжка к питанию
#define BUTTON_PIN 15

// время дребезга: механический контакт прыгает несколько миллисекунд,
// порог 20 мс перекрывает это время с запасом
#define BOUNCE_MS 20
#define BOUNCE_US (BOUNCE_MS * 1000)

// состояния кнопки: отпущена, ждём подтверждения нажатия, нажата,
// ждём подтверждения отпускания
typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESS_WAIT,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASE_WAIT,
} button_state_t;

static button_state_t button_state = BUTTON_STATE_RELEASED;
static uint64_t edge_us = 0;       // момент смены уровня, кандидат на переход
static uint32_t press_count = 0;
static bool pressed = false;       // кнопка нажата и в «нажата», и в «отпускание?»

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    button_state = BUTTON_STATE_RELEASED;
    pressed = false;
    press_count = 0;
    edge_us = time_us_64();
}

void button_task_handle(void)
{
    // читаем уровень: нажатая кнопка замыкает вывод на землю и даёт ноль
    bool level = (gpio_get(BUTTON_PIN) == 0);
    // читаем часы
    uint64_t now_us = time_us_64();

    switch (button_state)
    {
    case BUTTON_STATE_RELEASED:
        // новый уровень на выводе — запоминаем время и ждём подтверждения
        if (level)
        {
            edge_us = now_us;
            button_state = BUTTON_STATE_PRESS_WAIT;
        }
        break;

    case BUTTON_STATE_PRESS_WAIT:
        // уровень вернулся к нулю — это был дребезг или помеха
        if (!level)
        {
            button_state = BUTTON_STATE_RELEASED;
        }
        // уровень продержался 20 мс — нажатие подтверждено
        else if (now_us - edge_us >= BOUNCE_US)
        {
            button_state = BUTTON_STATE_PRESSED;
            pressed = true;
            press_count++;

            // каждое нажатие переводит светодиод в следующее состояние
            led_task_next_state();
        }
        break;

    case BUTTON_STATE_PRESSED:
        // уровень ушёл от нуля — запоминаем время и ждём подтверждения
        if (!level)
        {
            edge_us = now_us;
            button_state = BUTTON_STATE_RELEASE_WAIT;
        }
        break;

    case BUTTON_STATE_RELEASE_WAIT:
        // снова нажата — возвращаемся, отпускание не подтвердилось
        if (level)
        {
            button_state = BUTTON_STATE_PRESSED;
        }
        // отпускание продержалось 20 мс — кнопка отпущена
        else if (now_us - edge_us >= BOUNCE_US)
        {
            button_state = BUTTON_STATE_RELEASED;
            pressed = false;
        }
        break;
    }
}

// кнопка нажата и в состоянии «нажата», и в «отпускание?»:
// пока отпускание не подтвердилось, кнопку считают нажатой
bool button_task_is_pressed(void)
{
    return pressed;
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}
