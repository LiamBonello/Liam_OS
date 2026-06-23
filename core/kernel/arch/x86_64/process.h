#ifndef LIAM_OS_X86_64_PROCESS_H
#define LIAM_OS_X86_64_PROCESS_H

#include "types.h"

#define X86_64_PROCESS_MAX_PROCESSES 8U
#define X86_64_PROCESS_NAME_LEN 24U
#define X86_64_PROCESS_KERNEL_STACK_BYTES 4096ULL

typedef void (*x86_64_process_entry_t)(void *arg);

enum x86_64_process_state_code {
    X86_64_PROCESS_UNUSED = 0,
    X86_64_PROCESS_READY = 1,
    X86_64_PROCESS_RUNNING = 2,
    X86_64_PROCESS_EXITED = 3,
    X86_64_PROCESS_FAILED = 4
};

struct x86_64_process {
    u32 pid;
    u32 state;
    char name[X86_64_PROCESS_NAME_LEN];
    x86_64_process_entry_t entry;
    void *arg;
    u64 kernel_stack_base;
    u64 kernel_stack_top;
    u32 exit_code;
};

struct x86_64_process_smoke_state {
    u32 initialized;
    u32 table_capacity;
    u32 created_processes;
    u32 ready_before_run;
    u32 run_attempts;
    u32 run_count;
    u32 exited_processes;
    u32 failed_processes;
    u32 stack_allocations;
    u32 stack_alignment_ok;
    u32 stack_switches;
    u32 stack_execution_ok;
    u32 last_created_pid;
    u32 last_run_pid;
    u32 worker_a_count;
    u32 worker_b_count;
    u32 userland_foundation_ok;
    u32 syscall_dispatcher_ok;
    u32 smoke_ok;
    u64 first_stack_base;
    u64 first_stack_top;
    u64 second_stack_base;
    u64 second_stack_top;
    u64 worker_a_stack_sample;
    u64 worker_b_stack_sample;
};

void x86_64_process_initialize(struct x86_64_process_smoke_state *state);
u32 x86_64_process_create(const char *name,
                          x86_64_process_entry_t entry,
                          void *arg);
u32 x86_64_process_run_next_ready(void);
u32 x86_64_process_run_all_ready(u32 max_runs);
void x86_64_process_run_smoke(struct x86_64_process_smoke_state *state);
const struct x86_64_process_smoke_state *x86_64_process_get_smoke_state(void);

#endif
