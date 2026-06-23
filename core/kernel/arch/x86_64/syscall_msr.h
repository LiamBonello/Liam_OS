#ifndef LIAM_OS_X86_64_SYSCALL_MSR_H
#define LIAM_OS_X86_64_SYSCALL_MSR_H

#include "types.h"

struct x86_64_syscall_msr_state {
    u32 initialized;
    u32 fast_syscall_supported;
    u32 msr_supported;
    u32 efer_sce_enabled;
    u32 star_programmed;
    u32 lstar_programmed;
    u32 fmask_programmed;
    u32 msr_programmed;
    u32 msr_ok;
    u64 efer_before;
    u64 efer_after;
    u64 star_value;
    u64 lstar_value;
    u64 fmask_value;
};

void x86_64_syscall_msr_program(struct x86_64_syscall_msr_state *state,
                                u32 fast_syscall_supported,
                                u32 msr_supported);
void x86_64_syscall_msr_report(const struct x86_64_syscall_msr_state *state);

#endif
