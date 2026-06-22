#ifndef LIAM_OS_X86_64_CPUID_H
#define LIAM_OS_X86_64_CPUID_H

#include "types.h"

struct x86_64_cpuid_state {
    u32 cpuid_available;
    u32 max_basic_leaf;
    u32 max_extended_leaf;
    char vendor[13];
    u32 has_fpu;
    u32 has_msr;
    u32 has_apic;
    u32 has_sse;
    u32 has_sse2;
    u32 has_fast_syscall;
    u32 has_nx;
    u32 has_1g_pages;
    u32 has_long_mode;
    u32 hypervisor_present;
    u32 baseline_ok;
};

void x86_64_cpuid_state_init(struct x86_64_cpuid_state *state);

#endif
