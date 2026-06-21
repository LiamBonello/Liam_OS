#ifndef LIAM_OS_X86_64_IDT_H
#define LIAM_OS_X86_64_IDT_H

#include "types.h"

#define X86_64_IDT_DOUBLE_FAULT_VECTOR 8U
#define X86_64_IDT_DOUBLE_FAULT_IST 1U

struct x86_64_idt_state {
    u64 idtr_base;
    u16 idtr_limit;
    u32 exception_gates;
    u32 double_fault_vector;
    u32 double_fault_ist;
    u32 double_fault_present;
    u32 double_fault_ist_ok;
};

void x86_64_idt_init(void);
void x86_64_idt_get_state(struct x86_64_idt_state *state);
void x86_64_exception_handler(u64 vector, u64 error_code);

#endif
