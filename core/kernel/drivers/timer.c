#include "timer.h"
#include "../arch/i386/idt.h"
#include "../arch/i386/io.h"
#include "../core/log.h"
#include "../core/scheduler.h"

#define PIT_CHANNEL_0 0x40
#define PIT_COMMAND 0x43
#define PIT_BASE_FREQUENCY 1193182
#define IRQ_TIMER 0

static volatile uint32_t timer_ticks = 0;
static uint32_t timer_frequency_hz = 100;

static void timer_irq_handler(interrupt_registers_t *registers)
{
    (void)registers;
    timer_ticks++;

    scheduler_on_timer_tick(timer_ticks);
}

void timer_initialize(uint32_t frequency_hz)
{
    if (frequency_hz == 0)
    {
        frequency_hz = 100;
    }

    timer_frequency_hz = frequency_hz;

    uint32_t divisor = PIT_BASE_FREQUENCY / frequency_hz;

    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL_0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL_0, (uint8_t)((divisor >> 8) & 0xFF));

    idt_register_irq_handler(IRQ_TIMER, timer_irq_handler);

    log_success("PIT initialized");
    log_info_u32("Timer frequency Hz", frequency_hz);
}

uint32_t timer_get_ticks(void)
{
    return timer_ticks;
}

uint32_t timer_get_frequency_hz(void)
{
    return timer_frequency_hz;
}

void timer_wait_ticks(uint32_t ticks)
{
    uint32_t target = timer_ticks + ticks;

    while (timer_ticks < target)
    {
        __asm__ volatile("hlt");
    }
}