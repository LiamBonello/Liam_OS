#include "user_mode.h"

#include "console.h"
#include "process.h"
#include "syscall.h"
#include "userland.h"

u64 x86_64_user_mode_kernel_rsp;
u64 x86_64_user_mode_kernel_cr3;

static struct x86_64_user_mode_state *active_state;
static struct x86_64_syscall_dispatch_state active_dispatcher;
static u8 active_exec_user_code_page[X86_64_USER_PAGE_BYTES] __attribute__((aligned(4096)));

static void clear_state(struct x86_64_user_mode_state *state)
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

static void initialize_live_dispatcher(u32 current_pid)
{
    x86_64_syscall_dispatch_init(&active_dispatcher, current_pid);
    active_dispatcher.exec_user_code_page = (u64)active_exec_user_code_page;
    active_dispatcher.write_output_enabled = 1U;
    active_dispatcher.blocking_read_enabled = 1U;
}

static void reserve_shell_pid(void *arg)
{
    (void)arg;
}

static void halt_after_shell_exit(void)
{
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

u64 x86_64_user_mode_syscall_entry(struct x86_64_syscall_frame *frame)
{
    struct x86_64_user_mode_state *state = active_state;
    if (state == (struct x86_64_user_mode_state *)0 ||
        frame == (struct x86_64_syscall_frame *)0) {
        return X86_64_SYSCALL_RET_ENOSYS;
    }

    u64 result = x86_64_syscall_dispatch_frame(&active_dispatcher, frame);

    state->syscall_count = active_dispatcher.dispatch_calls;
    state->last_syscall = frame->number;
    state->last_result = result;
    state->last_user_rip = frame->user_rip;
    state->last_user_rflags = frame->user_rflags;
    state->live_dispatcher_initialized = active_dispatcher.initialized;
    state->syscall_frame_ok =
        ((frame->user_rip >= X86_64_USER_CODE_BASE) &&
         (frame->user_rip < X86_64_USER_HEAP_BASE) &&
         ((frame->user_rflags & 0x200ULL) != 0ULL)) ? 1U : 0U;

    switch (frame->number) {
    case X86_64_SYSCALL_SERVICE_GETPID:
        state->getpid_ok = (result == (u64)state->current_pid) ? 1U : 0U;
        break;

    case X86_64_SYSCALL_SERVICE_WRITE:
        state->write_ok = (result != X86_64_SYSCALL_RET_EFAULT &&
                           result != X86_64_SYSCALL_RET_EINVAL) ? 1U : 0U;
        break;

    case X86_64_SYSCALL_SERVICE_OPEN:
    case X86_64_SYSCALL_SERVICE_CLOSE:
    case X86_64_SYSCALL_SERVICE_STAT:
        break;

    case X86_64_SYSCALL_SERVICE_READ:
        state->read_ok = (result != X86_64_SYSCALL_RET_EFAULT &&
                          result != X86_64_SYSCALL_RET_EINVAL) ? 1U : 0U;
        break;

    case X86_64_SYSCALL_SERVICE_GET_ARG:
        state->get_arg_ok = (result != X86_64_SYSCALL_RET_EFAULT &&
                             result != X86_64_SYSCALL_RET_EINVAL) ? 1U : 0U;
        break;

    case X86_64_SYSCALL_SERVICE_EXEC:
        state->exec_ok = (result == X86_64_SYSCALL_RET_OK ||
                          result == X86_64_SYSCALL_RET_ENOSYS ||
                          result == X86_64_VFS_RET_ENOENT) ? 1U : 0U;
        if (result == X86_64_SYSCALL_RET_OK && active_dispatcher.exec_spawned_pid != 0U) {
            state->current_pid = active_dispatcher.exec_spawned_pid;
            active_dispatcher.current_pid = active_dispatcher.exec_spawned_pid;
        }
        break;

    case X86_64_SYSCALL_SERVICE_YIELD:
        state->yield_ok = ((result == X86_64_SYSCALL_RET_OK) &&
                           (active_dispatcher.yield_count >= 1U)) ? 1U : 0U;
        break;

    case X86_64_SYSCALL_SERVICE_EXIT:
        state->exit_ok = ((result == X86_64_SYSCALL_RET_OK) &&
                          (frame->exit_requested != 0U)) ? 1U : 0U;
        state->exit_code = active_dispatcher.exit_code;
        break;

    default:
        state->unexpected_syscall = 1U;
        break;
    }

    state->live_dispatcher_ok =
        ((active_dispatcher.initialized != 0U) &&
         (active_dispatcher.service_count == X86_64_SYSCALL_SERVICE_COUNT) &&
         (active_dispatcher.dispatch_calls >= 10U) &&
         (state->unexpected_syscall == 0U)) ? 1U : 0U;

    return result;
}

void x86_64_user_mode_start_init(struct x86_64_user_mode_state *state,
                                 const struct x86_64_paging_builder_state *paging_builder,
                                 u32 current_pid)
{
    struct x86_64_process_smoke_state process_state;

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

    x86_64_process_set_paging_context(paging_builder);
    x86_64_process_initialize(&process_state);
    if (current_pid == 1U) {
        (void)x86_64_process_create("shell", reserve_shell_pid, (void *)0);
    }

    initialize_live_dispatcher(current_pid);
    active_state = state;
    state->attempted = 1U;

    for (;;) {
        state->entered = 1U;
        x86_64_user_mode_enter_init(state->entry_rip, state->entry_rsp);
        u32 returned_pid = active_dispatcher.current_pid;
        state->returned_to_kernel = 1U;

        if (returned_pid == current_pid) {
            x86_64_serial_write_line("Liam_OS x86_64 shell exited");
            halt_after_shell_exit();
        }

        state->current_pid = current_pid;
        initialize_live_dispatcher(current_pid);
    }
}

void x86_64_user_mode_report(const struct x86_64_user_mode_state *state)
{
    x86_64_serial_write_line("x86_64 ring3 user mode online");
    x86_64_serial_write_u32("User mode initialized: ", state->initialized);
    x86_64_serial_write_u32("User mode attempted: ", state->attempted);
    x86_64_serial_write_u32("User mode entered: ", state->entered);
    x86_64_serial_write_u32("User mode returned to kernel: ", state->returned_to_kernel);
    x86_64_serial_write_u32("User mode syscall count: ", state->syscall_count);
    x86_64_serial_write_u32("User mode getpid ok: ", state->getpid_ok);
    x86_64_serial_write_u32("User mode write ok: ", state->write_ok);
    x86_64_serial_write_u32("User mode read ok: ", state->read_ok);
    x86_64_serial_write_u32("User mode get_arg ok: ", state->get_arg_ok);
    x86_64_serial_write_u32("User mode yield ok: ", state->yield_ok);
    x86_64_serial_write_u32("User mode exec ok: ", state->exec_ok);
    x86_64_serial_write_u32("User mode exit ok: ", state->exit_ok);
    x86_64_serial_write_u32("User mode unexpected syscall: ", state->unexpected_syscall);
    x86_64_serial_write_u32("User mode live dispatcher initialized: ", state->live_dispatcher_initialized);
    x86_64_serial_write_u32("User mode live dispatcher ok: ", state->live_dispatcher_ok);
    x86_64_serial_write_u32("User mode syscall frame ok: ", state->syscall_frame_ok);
    x86_64_serial_write_u32("User mode current pid: ", state->current_pid);
    x86_64_serial_write_u32("User mode exit code: ", state->exit_code);
    x86_64_serial_write_hex64("User mode entry RIP: 0x", state->entry_rip);
    x86_64_serial_write_hex64("User mode entry RSP: 0x", state->entry_rsp);
    x86_64_serial_write_hex64("User mode last syscall: 0x", state->last_syscall);
    x86_64_serial_write_hex64("User mode last result: 0x", state->last_result);
    x86_64_serial_write_hex64("User mode last user RIP: 0x", state->last_user_rip);
    x86_64_serial_write_hex64("User mode last user RFLAGS: 0x", state->last_user_rflags);
    x86_64_serial_write_u32("User mode entry ready: ", state->user_entry_ready);
    x86_64_serial_write_u32("User mode stack ready: ", state->user_stack_ready);
    x86_64_serial_write_u32("User mode ok: ", state->user_mode_ok);
}
