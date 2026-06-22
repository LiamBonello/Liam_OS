#ifndef LIAM_OS_X86_64_GDT_H
#define LIAM_OS_X86_64_GDT_H

#include "types.h"

#define X86_64_GDT_ENTRY_COUNT 7U
#define X86_64_GDT_CODE_SELECTOR 0x08U
#define X86_64_GDT_DATA_SELECTOR 0x10U
#define X86_64_GDT_TSS_SELECTOR 0x18U
#define X86_64_GDT_USER_DATA_SELECTOR 0x2BU
#define X86_64_GDT_USER_CODE_SELECTOR 0x33U
#define X86_64_GDT_SYSCALL_USER_SELECTOR_BASE 0x20U
#define X86_64_GDT_EXPECTED_LIMIT ((X86_64_GDT_ENTRY_COUNT * 8U) - 1U)

struct x86_64_tss_state;

struct x86_64_gdt_state {
    u64 gdtr_base;
    u16 gdtr_limit;
    u16 code_selector;
    u16 data_selector;
    u16 tss_selector;
    u16 user_data_selector;
    u16 user_code_selector;
    u16 syscall_user_selector_base;
    u16 current_cs;
    u16 current_ds;
    u16 current_ss;
    u16 current_tr;
    u64 null_descriptor;
    u64 code_descriptor;
    u64 data_descriptor;
    u64 tss_descriptor_low;
    u64 tss_descriptor_high;
    u64 user_data_descriptor;
    u64 user_code_descriptor;
    u32 entry_count;
    u32 limit_ok;
    u32 selectors_ok;
    u32 user_selectors_ok;
    u32 tss_loaded;
};

void x86_64_gdt_state_init(struct x86_64_gdt_state *state);
void x86_64_gdt_load_tss(const struct x86_64_tss_state *tss_state, struct x86_64_gdt_state *state);

#endif
