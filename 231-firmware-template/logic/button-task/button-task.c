#include "button-task/button-task.h"

#include <stddef.h>

#include "systime/systime.h"
#include "platform/platform.h"

// время дребезга: механический контакт прыгает несколько миллисекунд,
// порог 20 мс перекрывает это время с запасом (20000 мкс)
#define BOUNCE_US 20000

// состояния кнопки: отпущена, ждём подтверждения нажатия, нажата,
// ждём подтверждения отпускания
typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESS_WAIT,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASE_WAIT,
} button_state_t;

static button_task_callback_t on_press = NULL;
static button_state_t button_state = BUTTON_STATE_RELEASED;
static uint64_t edge_us = 0;       // момент смены уровня, кандидат на переход
static uint32_t press_count = 0;
static bool pressed = false;       // кнопка нажата и в «нажата», и в «отпускание?»

void button_task_init(button_task_callback_t callback)
{
    on_press = callback;

    button_state = BUTTON_STATE_RELEASED;
    pressed = false;
    press_count = 0;
    edge_us = systime_us();
}

void button_task_handle(void)
{
    // читаем уровень у платформы: она знает, что нажатие даёт ноль
    bool level = platform_button_read();
    // читаем часы
    uint64_t now_us = systime_us();

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

            // сообщаем о нажатии по адресу, полученному при запуске
            if (on_press != NULL)
            {
                on_press();
            }
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
