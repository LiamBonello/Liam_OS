#ifndef LIAM_OS_X86_64_USER_MODE_H
#define LIAM_OS_X86_64_USER_MODE_H

#include "paging_builder.h"
#include "syscall_dispatch.h"
#include "types.h"

#define X86_64_USER_MODE_SYSCALL_EXIT_SENTINEL 0xFFFFFFFFFFFFFFFFULL

struct x86_64_user_mode_state {
    u32 initialized;
    u32 attempted;
    u32 entered;
    u32 returned_to_kernel;
    u32 syscall_count;
    u32 getpid_ok;
    u32 write_ok;
    u32 read_ok;
    u32 get_arg_ok;
    u32 yield_ok;
    u32 exec_ok;
    u32 exit_ok;
    u32 unexpected_syscall;
    u32 live_dispatcher_initialized;
    u32 live_dispatcher_ok;
    u32 syscall_frame_ok;
    u32 user_entry_ready;
    u32 user_stack_ready;
    u32 user_mode_ok;
    u32 current_pid;
    u32 exit_code;
    u64 entry_rip;
    u64 entry_rsp;
    u64 last_syscall;
    u64 last_result;
    u64 last_user_rip;
    u64 last_user_rflags;
};

extern u64 x86_64_user_mode_kernel_rsp;
extern u64 x86_64_user_mode_kernel_cr3;

void x86_64_user_mode_enter_init(u64 entry_rip, u64 entry_rsp);
u64 x86_64_user_mode_syscall_entry(struct x86_64_syscall_frame *frame);
void x86_64_user_mode_start_init(struct x86_64_user_mode_state *state,
                                 const struct x86_64_paging_builder_state *paging_builder,
                                 u32 current_pid);
void x86_64_user_mode_report(const struct x86_64_user_mode_state *state);

#endif
