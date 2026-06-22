#ifndef LIAM_OS_X86_64_IDT_H
#define LIAM_OS_X86_64_IDT_H

#include "types.h"

#define X86_64_IDT_NMI_VECTOR 2U
#define X86_64_IDT_NMI_IST 2U
#define X86_64_IDT_DOUBLE_FAULT_VECTOR 8U
#define X86_64_IDT_DOUBLE_FAULT_IST 1U
#define X86_64_IDT_PAGE_FAULT_VECTOR 14U
#define X86_64_IDT_PAGE_FAULT_IST 3U

struct x86_64_idt_state {
    u64 idtr_base;
    u16 idtr_limit;
    u32 exception_gates;
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

void x86_64_idt_init(void);
void x86_64_idt_get_state(struct x86_64_idt_state *state);
void x86_64_exception_handler(u64 vector, u64 error_code);

#endif
