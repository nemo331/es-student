#pragma once

#include <stdbool.h>
#include <stdint.h>

// время с момента сброса
uint64_t systime_us(void);
uint64_t systime_ms(void);

// проверка периода: следующее срабатывание отсчитывается от прошлого,
// а не от текущего момента. Если прибор отстал больше чем на период,
// пропущенные срабатывания не догоняются, а отсчёт начинается заново
bool systime_period_elapsed(uint64_t *last_us, uint64_t period_us);
