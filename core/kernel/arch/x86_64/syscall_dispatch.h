#ifndef LIAM_OS_X86_64_SYSCALL_DISPATCH_H
#define LIAM_OS_X86_64_SYSCALL_DISPATCH_H

#include "console.h"
#include "idt.h"
#include "syscall.h"
#include "types.h"
#include "userland.h"

#define X86_64_SYSCALL_RET_OK 0ULL
#define X86_64_SYSCALL_RET_EFAULT 0xFFFFFFFFFFFFFFF2ULL
#define X86_64_SYSCALL_RET_EINVAL 0xFFFFFFFFFFFFFFEAULL
#define X86_64_SYSCALL_RET_ENOSYS 0xFFFFFFFFFFFFFFDAULL
#define X86_64_SYSCALL_STDIN 0ULL
#define X86_64_SYSCALL_STDOUT 1ULL
#define X86_64_SYSCALL_STDERR 2ULL
#define X86_64_SYSCALL_WRITE_MAX_BYTES 256ULL

struct x86_64_syscall_frame {
    u64 number;
    u64 arg0;
    u64 arg1;
    u64 arg2;
    u64 arg3;
    u64 arg4;
    u64 arg5;
    u64 user_rip;
    u64 user_rflags;
    u64 result;
    u32 exit_requested;
    u32 reserved;
};

struct x86_64_syscall_dispatch_state {
    u32 initialized;
    u32 service_count;
    u32 current_pid;
    u32 dispatch_calls;
    u32 pointer_validation_ok;
    u32 user_buffer_accepted;
    u32 kernel_pointer_rejected;
    u32 exit_ok;
    u32 write_ok;
    u32 write_fault_ok;
    u32 write_output_enabled;
    u32 read_ok;
    u32 read_fault_ok;
    u32 getpid_ok;
    u32 yield_ok;
    u32 exec_fault_ok;
    u32 unknown_rejected;
    u32 dispatcher_ok;
    u32 exit_code;
    u32 yield_count;
    u64 last_syscall;
    u64 last_result;
    u64 sample_user_buffer;
    u64 sample_kernel_pointer;
};

static inline u32 x86_64_syscall_user_range_ok(u64 address, u64 size)
{
    if (size == 0ULL) {
        return 1U;
    }

    if (address < X86_64_USER_CODE_BASE) {
        return 0U;
    }

    u64 end = address + size;
    if (end < address) {
        return 0U;
    }

    if (end > X86_64_USER_CANONICAL_LIMIT) {
        return 0U;
    }

    return 1U;
}

static inline void x86_64_syscall_write_serial(const char *buffer, u64 size)
{
    u64 limit = size;
    if (limit > X86_64_SYSCALL_WRITE_MAX_BYTES) {
        limit = X86_64_SYSCALL_WRITE_MAX_BYTES;
    }

    for (u64 i = 0ULL; i < limit; ++i) {
        char message[2];
        message[0] = buffer[i];
        message[1] = '\0';
        x86_64_serial_write(message);
    }
}

static inline void x86_64_syscall_dispatch_init(struct x86_64_syscall_dispatch_state *state,
                                                u32 current_pid)
{
    state->initialized = 1U;
    state->service_count = X86_64_SYSCALL_SERVICE_COUNT;
    state->current_pid = current_pid;
    state->dispatch_calls = 0U;
    state->pointer_validation_ok = 0U;
    state->user_buffer_accepted = 0U;
    state->kernel_pointer_rejected = 0U;
    state->exit_ok = 0U;
    state->write_ok = 0U;
    state->write_fault_ok = 0U;
    state->write_output_enabled = 0U;
    state->read_ok = 0U;
    state->read_fault_ok = 0U;
    state->getpid_ok = 0U;
    state->yield_ok = 0U;
    state->exec_fault_ok = 0U;
    state->unknown_rejected = 0U;
    state->dispatcher_ok = 0U;
    state->exit_code = 0U;
    state->yield_count = 0U;
    state->last_syscall = 0ULL;
    state->last_result = 0ULL;
    state->sample_user_buffer = X86_64_USER_CODE_BASE;
    state->sample_kernel_pointer = 0xFFFF800000100000ULL;
}

static inline u64 x86_64_syscall_dispatch(struct x86_64_syscall_dispatch_state *state,
                                          u64 syscall_number,
                                          u64 arg0,
                                          u64 arg1,
                                          u64 arg2,
                                          u64 arg3,
                                          u64 arg4,
                                          u64 arg5)
{
    (void)arg3;
    (void)arg4;
    (void)arg5;

    state->dispatch_calls += 1U;
    state->last_syscall = syscall_number;

    switch (syscall_number) {
    case X86_64_SYSCALL_SERVICE_EXIT:
        state->exit_code = (u32)arg0;
        state->last_result = X86_64_SYSCALL_RET_OK;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_WRITE:
        if (arg0 != X86_64_SYSCALL_STDOUT && arg0 != X86_64_SYSCALL_STDERR) {
            state->last_result = X86_64_SYSCALL_RET_EINVAL;
            return state->last_result;
        }
        if (x86_64_syscall_user_range_ok(arg1, arg2) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        if (state->write_output_enabled != 0U) {
            x86_64_syscall_write_serial((const char *)arg1, arg2);
        }
        state->last_result = arg2;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_READ:
        if (arg0 != X86_64_SYSCALL_STDIN) {
            state->last_result = X86_64_SYSCALL_RET_EINVAL;
            return state->last_result;
        }
        if (x86_64_syscall_user_range_ok(arg1, arg2) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        state->last_result = x86_64_keyboard_read((char *)arg1, arg2);
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_OPEN:
    case X86_64_SYSCALL_SERVICE_STAT:
    case X86_64_SYSCALL_SERVICE_GET_ARG:
    case X86_64_SYSCALL_SERVICE_EXEC:
        if (x86_64_syscall_user_range_ok(arg0, 1ULL) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        state->last_result = X86_64_SYSCALL_RET_ENOSYS;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_CLOSE:
        state->last_result = X86_64_SYSCALL_RET_ENOSYS;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_GETPID:
        state->last_result = (u64)state->current_pid;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_YIELD:
        state->yield_count += 1U;
        state->last_result = X86_64_SYSCALL_RET_OK;
        return state->last_result;

    default:
        state->last_result = X86_64_SYSCALL_RET_ENOSYS;
        return state->last_result;
    }
}

static inline u64 x86_64_syscall_dispatch_frame(struct x86_64_syscall_dispatch_state *state,
                                                struct x86_64_syscall_frame *frame)
{
    frame->exit_requested = 0U;
    frame->reserved = 0U;
    frame->result = x86_64_syscall_dispatch(state,
                                            frame->number,
                                            frame->arg0,
                                            frame->arg1,
                                            frame->arg2,
                                            frame->arg3,
                                            frame->arg4,
                                            frame->arg5);

    if (frame->number == X86_64_SYSCALL_SERVICE_EXIT && frame->result == X86_64_SYSCALL_RET_OK) {
        frame->exit_requested = 1U;
    }

    return frame->result;
}

static inline void x86_64_syscall_dispatch_run_smoke(struct x86_64_syscall_dispatch_state *state,
                                                     u32 current_pid)
{
    x86_64_syscall_dispatch_init(state, current_pid);

    state->user_buffer_accepted =
        x86_64_syscall_user_range_ok(state->sample_user_buffer, 4ULL);
    state->kernel_pointer_rejected =
        (x86_64_syscall_user_range_ok(state->sample_kernel_pointer, 4ULL) == 0U) ? 1U : 0U;
    state->pointer_validation_ok =
        ((state->user_buffer_accepted != 0U) &&
         (state->kernel_pointer_rejected != 0U)) ? 1U : 0U;

    u64 write_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_WRITE,
                                               X86_64_SYSCALL_STDOUT, state->sample_user_buffer, 4ULL,
                                               0ULL, 0ULL, 0ULL);
    state->write_ok = (write_result == 4ULL) ? 1U : 0U;

    u64 write_fault = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_WRITE,
                                              X86_64_SYSCALL_STDOUT, state->sample_kernel_pointer, 4ULL,
                                              0ULL, 0ULL, 0ULL);
    state->write_fault_ok = (write_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 read_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_READ,
                                              X86_64_SYSCALL_STDIN, state->sample_user_buffer, 4ULL,
                                              0ULL, 0ULL, 0ULL);
    state->read_ok = (read_result == X86_64_SYSCALL_RET_OK) ? 1U : 0U;

    u64 read_fault = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_READ,
                                             X86_64_SYSCALL_STDIN, state->sample_kernel_pointer, 4ULL,
                                             0ULL, 0ULL, 0ULL);
    state->read_fault_ok = (read_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 getpid_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_GETPID,
                                                0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    state->getpid_ok = (getpid_result == (u64)current_pid) ? 1U : 0U;

    u64 yield_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_YIELD,
                                               0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    state->yield_ok = ((yield_result == X86_64_SYSCALL_RET_OK) &&
                       (state->yield_count == 1U)) ? 1U : 0U;

    u64 exec_fault = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_EXEC,
                                             state->sample_kernel_pointer, 0ULL, 0ULL,
                                             0ULL, 0ULL, 0ULL);
    state->exec_fault_ok = (exec_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 unknown_result = x86_64_syscall_dispatch(state, 0xFFULL,
                                                 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    state->unknown_rejected = (unknown_result == X86_64_SYSCALL_RET_ENOSYS) ? 1U : 0U;

    u64 exit_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_EXIT,
                                              7ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    state->exit_ok = ((exit_result == X86_64_SYSCALL_RET_OK) &&
                      (state->exit_code == 7U)) ? 1U : 0U;

    state->dispatcher_ok =
        ((state->initialized != 0U) &&
         (state->service_count == X86_64_SYSCALL_SERVICE_COUNT) &&
         (state->pointer_validation_ok != 0U) &&
         (state->write_ok != 0U) &&
         (state->write_fault_ok != 0U) &&
         (state->read_ok != 0U) &&
         (state->read_fault_ok != 0U) &&
         (state->getpid_ok != 0U) &&
         (state->yield_ok != 0U) &&
         (state->exec_fault_ok != 0U) &&
         (state->unknown_rejected != 0U) &&
         (state->exit_ok != 0U) &&
         (state->dispatch_calls == 9U)) ? 1U : 0U;
}

static inline void x86_64_syscall_dispatch_report(const struct x86_64_syscall_dispatch_state *state)
{
    x86_64_serial_write_line("x86_64 syscall dispatcher online");
    x86_64_serial_write_u32("Syscall dispatcher initialized: ", state->initialized);
    x86_64_serial_write_u32("Syscall dispatcher service count: ", state->service_count);
    x86_64_serial_write_u32("Syscall dispatcher calls: ", state->dispatch_calls);
    x86_64_serial_write_hex64("Syscall sample user buffer: 0x", state->sample_user_buffer);
    x86_64_serial_write_hex64("Syscall sample kernel pointer: 0x", state->sample_kernel_pointer);
    x86_64_serial_write_u32("Syscall user buffer accepted: ", state->user_buffer_accepted);
    x86_64_serial_write_u32("Syscall kernel pointer rejected: ", state->kernel_pointer_rejected);
    x86_64_serial_write_u32("Syscall pointer validation ok: ", state->pointer_validation_ok);
    x86_64_serial_write_u32("Syscall write dispatch ok: ", state->write_ok);
    x86_64_serial_write_u32("Syscall write fault ok: ", state->write_fault_ok);
    x86_64_serial_write_u32("Syscall read dispatch ok: ", state->read_ok);
    x86_64_serial_write_u32("Syscall read fault ok: ", state->read_fault_ok);
    x86_64_serial_write_u32("Syscall getpid dispatch ok: ", state->getpid_ok);
    x86_64_serial_write_u32("Syscall yield dispatch ok: ", state->yield_ok);
    x86_64_serial_write_u32("Syscall exec fault ok: ", state->exec_fault_ok);
    x86_64_serial_write_u32("Syscall unknown rejected: ", state->unknown_rejected);
    x86_64_serial_write_u32("Syscall exit dispatch ok: ", state->exit_ok);
    x86_64_serial_write_u32("Syscall exit code: ", state->exit_code);
    x86_64_serial_write_u32("Syscall yield count: ", state->yield_count);
    x86_64_serial_write_hex64("Syscall last number: 0x", state->last_syscall);
    x86_64_serial_write_hex64("Syscall last result: 0x", state->last_result);
    x86_64_serial_write_u32("Syscall dispatcher ok: ", state->dispatcher_ok);
}

#endif
