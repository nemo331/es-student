#include "button-task/button-task.h"

#include <stddef.h>

#include "systime/systime.h"

// время дребезга: механический контакт прыгает несколько миллисекунд,
// порог 20 мс перекрывает это время с запасом (20000 мкс)
#define BOUNCE_US 20000

// порог долгого нажатия: удержание дольше секунды — долгое нажатие
#define LONG_PRESS_US 1000000

// состояния кнопки: отпущена, ждём подтверждения нажатия, нажата,
// ждём подтверждения отпускания
typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESS_WAIT,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASE_WAIT,
} button_state_t;

// машина одной кнопки
typedef struct
{
    button_state_t state;
    uint64_t edge_us;       // момент смены уровня, кандидат на переход
    uint64_t press_us;      // момент подтверждённого нажатия
    bool long_reported;     // о долгом нажатии уже сообщили
    uint32_t press_count;
    bool pressed;
} button_machine_t;

static button_machine_t machines[PLATFORM_BUTTON_COUNT];
static button_task_callback_t short_callbacks[PLATFORM_BUTTON_COUNT];
static button_task_callback_t long_callbacks[PLATFORM_BUTTON_COUNT];

void button_task_init(platform_button_t button,
                      button_task_callback_t on_press,
                      button_task_callback_t on_long_press)
{
    if (button >= PLATFORM_BUTTON_COUNT)
    {
        return;
    }

    short_callbacks[button] = on_press;
    long_callbacks[button] = on_long_press;

    machines[button].state = BUTTON_STATE_RELEASED;
    machines[button].pressed = false;
    machines[button].press_count = 0;
    machines[button].long_reported = false;
    machines[button].edge_us = systime_us();
    machines[button].press_us = 0;
}

void button_task_handle(void)
{
    uint64_t now_us = systime_us();

    for (uint32_t i = 0; i < PLATFORM_BUTTON_COUNT; i++)
    {
        button_machine_t *machine = &machines[i];

        // читаем уровень у платформы: она знает, что нажатие даёт ноль
        bool level = platform_button_read((platform_button_t)i);

        switch (machine->state)
        {
        case BUTTON_STATE_RELEASED:
            // новый уровень на выводе — запоминаем время и ждём подтверждения
            if (level)
            {
                machine->edge_us = now_us;
                machine->state = BUTTON_STATE_PRESS_WAIT;
            }
            break;

        case BUTTON_STATE_PRESS_WAIT:
            // уровень вернулся к нулю — это был дребезг или помеха
            if (!level)
            {
                machine->state = BUTTON_STATE_RELEASED;
            }
            // уровень продержался 20 мс — нажатие подтверждено
            else if (now_us - machine->edge_us >= BOUNCE_US)
            {
                machine->state = BUTTON_STATE_PRESSED;
                machine->press_us = now_us;
                machine->pressed = true;
                machine->press_count++;
                machine->long_reported = false;

                // у кнопки без долгого действия короткое нажатие известно
                // сразу; с долгим действием ждать отпускания — в момент
                // нажатия ещё неизвестно, станет ли оно долгим
                if (long_callbacks[i] == NULL && short_callbacks[i] != NULL)
                {
                    short_callbacks[i]();
                }
            }
            break;

        case BUTTON_STATE_PRESSED:
            // уровень ушёл от нуля — запоминаем время и ждём подтверждения
            if (!level)
            {
                machine->edge_us = now_us;
                machine->state = BUTTON_STATE_RELEASE_WAIT;
            }
            // кнопку держат дольше секунды — долгое нажатие, один раз
            else if (long_callbacks[i] != NULL &&
                     !machine->long_reported &&
                     now_us - machine->press_us >= LONG_PRESS_US)
            {
                machine->long_reported = true;
                long_callbacks[i]();
            }
            break;

        case BUTTON_STATE_RELEASE_WAIT:
            // снова нажата — возвращаемся, отпускание не подтвердилось
            if (level)
            {
                machine->state = BUTTON_STATE_PRESSED;
            }
            // отпускание продержалось 20 мс — кнопка отпущена
            else if (now_us - machine->edge_us >= BOUNCE_US)
            {
                machine->state = BUTTON_STATE_RELEASED;
                machine->pressed = false;

                // у кнопки с долгим действием короткое сообщается при
                // отпускании: теперь известно, что секунды не случилось
                if (long_callbacks[i] != NULL &&
                    !machine->long_reported &&
                    short_callbacks[i] != NULL)
                {
                    short_callbacks[i]();
                }

                machine->long_reported = false;
            }
            break;
        }
    }
}

// кнопка нажата и в состоянии «нажата», и в «отпускание?»:
// пока отпускание не подтвердилось, кнопку считают нажатой
bool button_task_is_pressed(platform_button_t button)
{
    if (button >= PLATFORM_BUTTON_COUNT)
    {
        return false;
    }

    return machines[button].pressed;
}

uint32_t button_task_get_press_count(platform_button_t button)
{
    if (button >= PLATFORM_BUTTON_COUNT)
    {
        return 0;
    }

    return machines[button].press_count;
}
