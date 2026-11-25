#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "log.h"

#include "clock.h"

// строка таблицы: имя сигнала, настроенная частота, измеренная частота (кГц)
static void row(const char *name, uint32_t set_khz, uint32_t measured_khz)
{
    printf("%-8s %9u %12u\n", name, (unsigned)set_khz, (unsigned)measured_khz);
}

// пониженная частота ядра: вдвое ниже рабочей
const uint32_t CLK_SYS_LOW_KHZ = 62500;

// почему после смены частоты clk_peri работает на 48 МГц:
// set_sys_clock_khz() вызывает set_sys_clock_pll() в исходниках SDK
// (hardware_clocks/clocks.c). Тот перед перестройкой PLL_SYS сначала
// временно переводит clk_sys на clk_ref, а чтобы периферия (UART, SPI),
// которая берёт clk_peri, не осталась без такта, перецепляет clk_peri
// на PLL_USB с его постоянными 48 МГц — там clk_peri и остаётся после
// смены частоты ядра. Сама смена частоты затрагивает только clk_sys.

// ставит частоту ядра и сообщает результат; при неудаче не меняет ничего
static void clk_sys_set(uint32_t khz)
{
    if (set_sys_clock_khz(khz, false))
    {
        LOG_INF("clk_sys %u kHz\n", (unsigned)khz);
    }
    else
    {
        LOG_ERR("clk_sys %u kHz is not set\n", (unsigned)khz);
    }
}

void clk_sys_low(void)
{
    clk_sys_set(CLK_SYS_LOW_KHZ);
}

void clk_sys_default(void)
{
    clk_sys_set(SYS_CLK_KHZ);
}

// паспорт тактовых сигналов: настроено и измерено
void clk_info(void)
{
    printf("%-8s %9s %12s\n", "signal", "set khz", "measured khz");

    row("clk_ref", clock_get_hz(clk_ref) / 1000,
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_REF));
    row("clk_sys", clock_get_hz(clk_sys) / 1000,
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS));
    row("clk_peri", clock_get_hz(clk_peri) / 1000,
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI));
    row("clk_usb", clock_get_hz(clk_usb) / 1000,
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB));
    row("clk_adc", clock_get_hz(clk_adc) / 1000,
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC));

    // rosc — у него нет настроенной частоты, SDK её не хранит: прочерк и измерение
    printf("%-8s %9s %12u\n", "rosc", "-",
           (unsigned)frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC));
}

void uptime(void)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

// ——— измерение скорости: расчёт числа пи рядом Лейбница ———

// итераций в одном замере: при 125 МГц это десятки миллисекунд
#define SPEED_ITERATIONS 10000u

// расчёт числа пи. Результат возвращаем и печатаем в замере:
// иначе компилятор вправе выбросить весь цикл как неиспользуемый,
// и замер будет мерить пустоту — обе частоты дадут одинаковое время
static float calc_pi(uint32_t iterations)
{
    float pi = 0.0f;

    for (uint32_t i = 0; i < iterations; i++)
    {
        pi += ((i & 1u) ? -4.0f : 4.0f) / (float)(2 * i + 1);
    }

    return pi;
}

// три замера подряд, каждый — в лог; средняя итерация по всем трём
void clk_speed(void)
{
    // фактическая частота ядра: по ней сразу видно, состоялся ли перевод
    LOG_INF("clk_speed at %u kHz\n",
            (unsigned)frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS));

    // прогрев: код исполняется из флеш через кэш XIP, первый прогон всегда
    // медленнее — в замер его не включаем
    calc_pi(SPEED_ITERATIONS);

    uint32_t total_us = 0;

    for (uint run = 1; run <= 3; run++)
    {
        uint64_t start_us = time_us_64();
        float pi = calc_pi(SPEED_ITERATIONS);
        uint32_t elapsed_us = (uint32_t)(time_us_64() - start_us);
        total_us += elapsed_us;

        // средняя итерация с двумя знаками без printf-дробей:
        // avg_x100 = elapsed * 100 / iterations, дальше целая и дробная части
        uint32_t avg_x100 = elapsed_us * 100u / SPEED_ITERATIONS;

        LOG_INF("calc_pi run %u: %u us, avg %u.%02u us, pi %u.%03u\n",
                run,
                elapsed_us,
                avg_x100 / 100u, avg_x100 % 100u,
                (unsigned)pi, (unsigned)(pi * 1000.0f) % 1000u);
    }

    // средняя итерация по трём замерам — без слова calc_pi в имени,
    // чтобы строку не приняли за четвёртый замер
    uint32_t avg_all_x100 = total_us * 100u / (3u * SPEED_ITERATIONS);
    LOG_INF("clk_speed avg: %u.%02u us\n",
            avg_all_x100 / 100u, avg_all_x100 % 100u);
}
