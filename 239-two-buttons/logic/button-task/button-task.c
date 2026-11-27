#include "button-task/button-task.h"

#include <stddef.h>

#include "systime/systime.h"

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

// машина одной кнопки: состояние, время смены уровня, счётчик нажатий
typedef struct
{
    button_state_t state;
    uint64_t edge_us;
    uint32_t press_count;
    bool pressed;
} button_machine_t;

static button_machine_t machines[PLATFORM_BUTTON_COUNT];
static button_task_callback_t callbacks[PLATFORM_BUTTON_COUNT];

void button_task_init(platform_button_t button, button_task_callback_t on_press)
{
    if (button >= PLATFORM_BUTTON_COUNT)
    {
        return;
    }

    callbacks[button] = on_press;

    machines[button].state = BUTTON_STATE_RELEASED;
    machines[button].pressed = false;
    machines[button].press_count = 0;
    machines[button].edge_us = systime_us();
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
                machine->pressed = true;
                machine->press_count++;

                // сообщаем о нажатии по адресу, полученному при запуске
                if (callbacks[i] != NULL)
                {
                    callbacks[i]();
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
