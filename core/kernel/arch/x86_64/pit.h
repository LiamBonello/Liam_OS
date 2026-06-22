#ifndef LIAM_OS_X86_64_PIT_H
#define LIAM_OS_X86_64_PIT_H

#include "types.h"

#define X86_64_PIT_DEFAULT_FREQUENCY_HZ 100U
#define X86_64_PIT_IRQ_VECTOR 32U

struct x86_64_pit_state {
    u32 initialized;
    u32 interrupts_enabled_before;
    u32 interrupts_enabled_after;
    u32 irq0_unmasked;
    u32 frequency_hz;
    u32 divisor;
    u32 ticks;
    u32 waited_ticks;
    u32 wait_ok;
    u32 timer_ok;
};

void x86_64_pit_initialize(u32 frequency_hz);
u32 x86_64_pit_handle_irq(u64 vector);
void x86_64_pit_wait_for_ticks(u32 ticks);
void x86_64_pit_get_state(struct x86_64_pit_state *state);

#endif
