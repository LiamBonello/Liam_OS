#include "pit.h"

#define X86_64_PIT_CHANNEL_0 0x40U
#define X86_64_PIT_COMMAND 0x43U
#define X86_64_PIT_BASE_FREQUENCY 1193182U
#define X86_64_PIT_MODE_RATE_GENERATOR 0x36U
#define X86_64_PIC1_DATA 0x21U
#define X86_64_RFLAGS_INTERRUPT_ENABLE (1ULL << 9U)

static volatile u32 pit_ticks;
static struct x86_64_pit_state pit_state;

static u8 inb(u16 port)
{
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a" (value) : "Nd" (port));
    return value;
}

static void outb(u16 port, u8 value)
{
    __asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (port));
}

static u64 read_rflags(void)
{
    u64 value;
    __asm__ volatile ("pushfq; popq %0" : "=r" (value));
    return value;
}

static void enable_interrupts(void)
{
    __asm__ volatile ("sti" ::: "memory");
}

static void unmask_irq0(void)
{
    u8 mask = inb(X86_64_PIC1_DATA);
    mask = (u8)(mask & ~1U);
    outb(X86_64_PIC1_DATA, mask);
    pit_state.irq0_unmasked = ((inb(X86_64_PIC1_DATA) & 1U) == 0U) ? 1U : 0U;
}

void x86_64_pit_initialize(u32 frequency_hz)
{
    if (frequency_hz == 0U) {
        frequency_hz = X86_64_PIT_DEFAULT_FREQUENCY_HZ;
    }

    u32 divisor = X86_64_PIT_BASE_FREQUENCY / frequency_hz;
    if (divisor == 0U) {
        divisor = 1U;
    }

    pit_ticks = 0U;
    pit_state.initialized = 1U;
    pit_state.frequency_hz = frequency_hz;
    pit_state.divisor = divisor;
    pit_state.ticks = 0U;
    pit_state.waited_ticks = 0U;
    pit_state.wait_ok = 0U;
    pit_state.timer_ok = 0U;
    pit_state.interrupts_enabled_before = ((read_rflags() & X86_64_RFLAGS_INTERRUPT_ENABLE) != 0ULL) ? 1U : 0U;

    outb(X86_64_PIT_COMMAND, X86_64_PIT_MODE_RATE_GENERATOR);
    outb(X86_64_PIT_CHANNEL_0, (u8)(divisor & 0xFFU));
    outb(X86_64_PIT_CHANNEL_0, (u8)((divisor >> 8U) & 0xFFU));

    unmask_irq0();
    enable_interrupts();

    pit_state.interrupts_enabled_after = ((read_rflags() & X86_64_RFLAGS_INTERRUPT_ENABLE) != 0ULL) ? 1U : 0U;
}

u32 x86_64_pit_handle_irq(u64 vector)
{
    if (vector != X86_64_PIT_IRQ_VECTOR) {
        return 0U;
    }

    pit_ticks += 1U;
    pit_state.ticks = pit_ticks;
    return 1U;
}

void x86_64_pit_wait_for_ticks(u32 ticks)
{
    u32 start = pit_ticks;
    u32 target = start + ticks;
    u32 spin_budget = 200000000U;

    while (pit_ticks < target && spin_budget > 0U) {
        --spin_budget;
        __asm__ volatile ("hlt" ::: "memory");
    }

    pit_state.ticks = pit_ticks;
    pit_state.waited_ticks = pit_ticks - start;
    pit_state.wait_ok = (pit_state.waited_ticks >= ticks) ? 1U : 0U;
    pit_state.timer_ok = ((pit_state.initialized != 0U) &&
                          (pit_state.irq0_unmasked != 0U) &&
                          (pit_state.interrupts_enabled_after != 0U) &&
                          (pit_state.wait_ok != 0U)) ? 1U : 0U;
}

void x86_64_pit_get_state(struct x86_64_pit_state *state)
{
    pit_state.ticks = pit_ticks;
    *state = pit_state;
}
