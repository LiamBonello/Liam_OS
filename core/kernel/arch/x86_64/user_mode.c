#include "user_mode.h"

#include "console.h"
#include "syscall.h"
#include "userland.h"

u64 x86_64_user_smoke_kernel_rsp;

static struct x86_64_user_mode_smoke_state *active_state;

static void clear_state(struct x86_64_user_mode_smoke_state *state)
{
    u8 *bytes = (u8 *)state;
    for (usize i = 0; i < sizeof(*state); ++i) {
        bytes[i] = 0U;
    }
}

static u32 is_user_entry_ready(const struct x86_64_paging_builder_state *paging_builder)
{
    return ((paging_builder->builder_ok != 0U) &&
            (paging_builder->user_mapping_ok != 0U) &&
            (paging_builder->user_image_loaded != 0U) &&
            (paging_builder->elf64_load_ok != 0U) &&
            (paging_builder->user_code_virtual == X86_64_USER_CODE_BASE)) ? 1U : 0U;
}

static u32 is_user_stack_ready(const struct x86_64_paging_builder_state *paging_builder)
{
    return ((paging_builder->builder_ok != 0U) &&
            (paging_builder->user_stack_entry_ok != 0U) &&
            (paging_builder->user_stack_zeroed != 0U) &&
            (paging_builder->user_stack_virtual == (X86_64_USER_STACK_TOP - X86_64_USER_PAGE_BYTES))) ? 1U : 0U;
}

u64 x86_64_user_smoke_syscall(u64 syscall_number, u64 arg0)
{
    struct x86_64_user_mode_smoke_state *state = active_state;
    if (state == (struct x86_64_user_mode_smoke_state *)0) {
        return X86_64_USER_MODE_SYSCALL_EXIT_SENTINEL;
    }

    state->syscall_count += 1U;
    state->last_syscall = syscall_number;

    switch (syscall_number) {
    case X86_64_SYSCALL_SERVICE_GETPID:
        state->getpid_ok = 1U;
        state->last_result = (u64)state->current_pid;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_YIELD:
        state->yield_ok = 1U;
        state->last_result = 0ULL;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_EXIT:
        state->exit_ok = 1U;
        state->exit_code = (u32)arg0;
        state->last_result = X86_64_USER_MODE_SYSCALL_EXIT_SENTINEL;
        return state->last_result;

    default:
        state->unexpected_syscall = 1U;
        state->last_result = X86_64_USER_MODE_SYSCALL_EXIT_SENTINEL;
        return state->last_result;
    }
}

void x86_64_user_mode_run_smoke(struct x86_64_user_mode_smoke_state *state,
                                const struct x86_64_paging_builder_state *paging_builder,
                                u32 current_pid)
{
    clear_state(state);
    state->initialized = 1U;
    state->current_pid = current_pid;
    state->entry_rip = X86_64_USER_CODE_BASE;
    state->entry_rsp = X86_64_USER_STACK_TOP - 16ULL;
    state->user_entry_ready = is_user_entry_ready(paging_builder);
    state->user_stack_ready = is_user_stack_ready(paging_builder);

    if (state->user_entry_ready == 0U || state->user_stack_ready == 0U) {
        return;
    }

    active_state = state;
    state->attempted = 1U;
    state->entered = 1U;
    x86_64_user_smoke_enter(state->entry_rip, state->entry_rsp);
    state->returned_to_kernel = 1U;
    active_state = (struct x86_64_user_mode_smoke_state *)0;

    state->user_mode_ok =
        ((state->initialized != 0U) &&
         (state->attempted != 0U) &&
         (state->entered != 0U) &&
         (state->returned_to_kernel != 0U) &&
         (state->syscall_count == 3U) &&
         (state->getpid_ok != 0U) &&
         (state->yield_ok != 0U) &&
         (state->exit_ok != 0U) &&
         (state->unexpected_syscall == 0U) &&
         (state->exit_code == 0U) &&
         (state->user_entry_ready != 0U) &&
         (state->user_stack_ready != 0U)) ? 1U : 0U;
}

void x86_64_user_mode_report(const struct x86_64_user_mode_smoke_state *state)
{
    x86_64_serial_write_line("x86_64 ring3 user smoke online");
    x86_64_serial_write_u32("User mode initialized: ", state->initialized);
    x86_64_serial_write_u32("User mode attempted: ", state->attempted);
    x86_64_serial_write_u32("User mode entered: ", state->entered);
    x86_64_serial_write_u32("User mode returned to kernel: ", state->returned_to_kernel);
    x86_64_serial_write_u32("User mode syscall count: ", state->syscall_count);
    x86_64_serial_write_u32("User mode getpid ok: ", state->getpid_ok);
    x86_64_serial_write_u32("User mode yield ok: ", state->yield_ok);
    x86_64_serial_write_u32("User mode exit ok: ", state->exit_ok);
    x86_64_serial_write_u32("User mode unexpected syscall: ", state->unexpected_syscall);
    x86_64_serial_write_u32("User mode current pid: ", state->current_pid);
    x86_64_serial_write_u32("User mode exit code: ", state->exit_code);
    x86_64_serial_write_hex64("User mode entry RIP: 0x", state->entry_rip);
    x86_64_serial_write_hex64("User mode entry RSP: 0x", state->entry_rsp);
    x86_64_serial_write_hex64("User mode last syscall: 0x", state->last_syscall);
    x86_64_serial_write_hex64("User mode last result: 0x", state->last_result);
    x86_64_serial_write_u32("User mode entry ready: ", state->user_entry_ready);
    x86_64_serial_write_u32("User mode stack ready: ", state->user_stack_ready);
    x86_64_serial_write_u32("User mode ok: ", state->user_mode_ok);
}
