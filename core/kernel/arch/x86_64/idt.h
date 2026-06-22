#ifndef LIAM_OS_X86_64_IDT_H
#define LIAM_OS_X86_64_IDT_H

#include "types.h"

#define X86_64_IDT_NMI_VECTOR 2U
#define X86_64_IDT_NMI_IST 2U
#define X86_64_IDT_DOUBLE_FAULT_VECTOR 8U
#define X86_64_IDT_DOUBLE_FAULT_IST 1U
#define X86_64_IDT_PAGE_FAULT_VECTOR 14U
#define X86_64_IDT_PAGE_FAULT_IST 3U
#define X86_64_IRQ_VECTOR_BASE 32U
#define X86_64_IRQ_VECTOR_COUNT 16U
#define X86_64_IRQ_PIT_VECTOR 32U
#define X86_64_IRQ_KEYBOARD_VECTOR 33U
#define X86_64_PIT_DEFAULT_FREQUENCY_HZ 100U

struct x86_64_idt_state {
    u64 idtr_base;
    u16 idtr_limit;
    u32 exception_gates;
    u32 irq_gates;
    u32 irq_vector_base;
    u32 irq_vector_count;
    u32 irq_pit_vector;
    u32 irq_keyboard_vector;
    u32 irq_gates_ok;
    u32 irq_policy_ok;
    u32 legacy_pic_remapped;
    u32 legacy_pic_all_masked;
    u32 legacy_pic_master_mask;
    u32 legacy_pic_slave_mask;
    u32 nmi_vector;
    u32 nmi_ist;
    u32 nmi_present;
    u32 nmi_ist_ok;
    u32 double_fault_vector;
    u32 double_fault_ist;
    u32 double_fault_present;
    u32 double_fault_ist_ok;
    u32 page_fault_vector;
    u32 page_fault_ist;
    u32 page_fault_present;
    u32 page_fault_ist_ok;
    u32 page_fault_cr2_reporting;
    u32 page_fault_error_decode;
    u32 panic_halt_ready;
    u32 panic_cli_before_hlt;
    u32 diagnostics_ok;
    u32 ist_gates_ok;
};

struct x86_64_timer_state {
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

void x86_64_idt_init(void);
void x86_64_idt_get_state(struct x86_64_idt_state *state);
void x86_64_timer_initialize(u32 frequency_hz);
void x86_64_timer_wait_for_ticks(u32 ticks);
void x86_64_timer_get_state(struct x86_64_timer_state *state);
void x86_64_exception_handler(u64 vector, u64 error_code);
void x86_64_irq_handler(u64 vector);

#endif
