#ifndef LIAM_OS_X86_64_USER_MODE_H
#define LIAM_OS_X86_64_USER_MODE_H

#include "paging_builder.h"
#include "types.h"

#define X86_64_USER_MODE_SYSCALL_EXIT_SENTINEL 0xFFFFFFFFFFFFFFFFULL

struct x86_64_user_mode_smoke_state {
    u32 initialized;
    u32 attempted;
    u32 entered;
    u32 returned_to_kernel;
    u32 syscall_count;
    u32 getpid_ok;
    u32 yield_ok;
    u32 exit_ok;
    u32 unexpected_syscall;
    u32 user_entry_ready;
    u32 user_stack_ready;
    u32 user_mode_ok;
    u32 current_pid;
    u32 exit_code;
    u64 entry_rip;
    u64 entry_rsp;
    u64 last_syscall;
    u64 last_result;
};

extern u64 x86_64_user_smoke_kernel_rsp;

void x86_64_user_smoke_enter(u64 entry_rip, u64 entry_rsp);
u64 x86_64_user_smoke_syscall(u64 syscall_number, u64 arg0);
void x86_64_user_mode_run_smoke(struct x86_64_user_mode_smoke_state *state,
                                const struct x86_64_paging_builder_state *paging_builder,
                                u32 current_pid);
void x86_64_user_mode_report(const struct x86_64_user_mode_smoke_state *state);

#endif
