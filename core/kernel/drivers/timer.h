#ifndef LIAM_OS_TIMER_H
#define LIAM_OS_TIMER_H

#include "../core/types.h"


void timer_initialize(uint32_t frequency_hz);
uint32_t timer_get_ticks(void);
uint32_t timer_get_frequency_hz(void);
void timer_wait_ticks(uint32_t ticks);

#endif