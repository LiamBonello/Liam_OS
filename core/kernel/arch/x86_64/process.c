#include "process.h"

#include "console.h"
#include "heap.h"
#include "userland.h"

#define X86_64_PROCESS_INVALID_PID 0U
#define X86_64_PROCESS_FIRST_PID 1U

__attribute__((naked)) static void x86_64_process_enter_stack(
    u64 stack_top __attribute__((unused)),
    x86_64_process_entry_t entry __attribute__((unused)),
    void *arg __attribute__((unused)))
{
    __asm__ volatile (
        "push %rbx\n"
        "mov %rsp, %rbx\n"
        "mov %rdi, %rsp\n"
        "and $-16, %rsp\n"
        "mov %rdx, %rdi\n"
        "call *%rsi\n"
        "mov %rbx, %rsp\n"
        "pop %rbx\n"
        "ret\n"
    );
}

static struct x86_64_process process_table[X86_64_PROCESS_MAX_PROCESSES];
static struct x86_64_process_smoke_state process_state;
static u32 next_pid = X86_64_PROCESS_FIRST_PID;
static u32 worker_a_runs;
static u32 worker_b_runs;
static u64 worker_a_stack_sample;
static u64 worker_b_stack_sample;

static void clear_bytes(void *ptr, usize size)
{
    u8 *bytes = (u8 *)ptr;
    for (usize i = 0; i < size; ++i) {
        bytes[i] = 0U;
    }
}

static u64 read_rsp(void)
{
    u64 value;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(value));
    return value;
}

static u32 is_aligned_u64(u64 value, u64 alignment)
{
    return ((value & (alignment - 1ULL)) == 0ULL) ? 1U : 0U;
}

static u32 is_inside_stack(u64 value, u64 stack_base, u64 stack_top)
{
    return ((value >= stack_base) && (value < stack_top)) ? 1U : 0U;
}

static void copy_name(char *dest, const char *src)
{
    u32 i = 0U;
    if (src != (const char *)0) {
        while (i + 1U < X86_64_PROCESS_NAME_LEN && src[i] != '\0') {
            dest[i] = src[i];
            i += 1U;
        }
    }

    dest[i] = '\0';
}

static struct x86_64_process *find_unused_process(void)
{
    for (u32 i = 0; i < X86_64_PROCESS_MAX_PROCESSES; ++i) {
        if (process_table[i].state == X86_64_PROCESS_UNUSED) {
            return &process_table[i];
        }
    }

    return (struct x86_64_process *)0;
}

static struct x86_64_process *find_next_ready_process(void)
{
    for (u32 i = 0; i < X86_64_PROCESS_MAX_PROCESSES; ++i) {
        if (process_table[i].state == X86_64_PROCESS_READY) {
            return &process_table[i];
        }
    }

    return (struct x86_64_process *)0;
}

static u32 count_processes_with_state(u32 state)
{
    u32 count = 0U;
    for (u32 i = 0; i < X86_64_PROCESS_MAX_PROCESSES; ++i) {
        if (process_table[i].state == state) {
            count += 1U;
        }
    }

    return count;
}

static void refresh_counts(void)
{
    process_state.exited_processes = count_processes_with_state(X86_64_PROCESS_EXITED);
    process_state.failed_processes = count_processes_with_state(X86_64_PROCESS_FAILED);
    process_state.worker_a_count = worker_a_runs;
    process_state.worker_b_count = worker_b_runs;
    process_state.worker_a_stack_sample = worker_a_stack_sample;
    process_state.worker_b_stack_sample = worker_b_stack_sample;
}

void x86_64_process_initialize(struct x86_64_process_smoke_state *state)
{
    clear_bytes(process_table, sizeof(process_table));
    clear_bytes(&process_state, sizeof(process_state));
    next_pid = X86_64_PROCESS_FIRST_PID;
    worker_a_runs = 0U;
    worker_b_runs = 0U;
    worker_a_stack_sample = 0ULL;
    worker_b_stack_sample = 0ULL;

    process_state.initialized = 1U;
    process_state.table_capacity = X86_64_PROCESS_MAX_PROCESSES;

    if (state != &process_state) {
        *state = process_state;
    }
}

u32 x86_64_process_create(const char *name,
                          x86_64_process_entry_t entry,
                          void *arg)
{
    if (process_state.initialized == 0U || entry == (x86_64_process_entry_t)0) {
        return X86_64_PROCESS_INVALID_PID;
    }

    struct x86_64_process *process = find_unused_process();
    if (process == (struct x86_64_process *)0) {
        return X86_64_PROCESS_INVALID_PID;
    }

    void *stack = x86_64_heap_alloc((usize)X86_64_PROCESS_KERNEL_STACK_BYTES,
                                    (usize)X86_64_PROCESS_KERNEL_STACK_BYTES);
    if (stack == (void *)0) {
        return X86_64_PROCESS_INVALID_PID;
    }

    process->pid = next_pid;
    next_pid += 1U;
    process->state = X86_64_PROCESS_READY;
    copy_name(process->name, name);
    process->entry = entry;
    process->arg = arg;
    process->kernel_stack_base = (u64)stack;
    process->kernel_stack_top = process->kernel_stack_base + X86_64_PROCESS_KERNEL_STACK_BYTES;
    process->exit_code = 0U;

    process_state.created_processes += 1U;
    process_state.stack_allocations += 1U;
    process_state.last_created_pid = process->pid;

    if (process_state.first_stack_base == 0ULL) {
        process_state.first_stack_base = process->kernel_stack_base;
        process_state.first_stack_top = process->kernel_stack_top;
    } else if (process_state.second_stack_base == 0ULL) {
        process_state.second_stack_base = process->kernel_stack_base;
        process_state.second_stack_top = process->kernel_stack_top;
    }

    process_state.stack_alignment_ok =
        ((process->kernel_stack_base != 0ULL) &&
         (is_aligned_u64(process->kernel_stack_base, 16ULL) != 0U) &&
         (is_aligned_u64(process->kernel_stack_top, 16ULL) != 0U)) ? 1U : 0U;

    return process->pid;
}

u32 x86_64_process_run_next_ready(void)
{
    struct x86_64_process *process = find_next_ready_process();
    if (process == (struct x86_64_process *)0) {
        return 0U;
    }

    process_state.run_attempts += 1U;
    process_state.last_run_pid = process->pid;
    process->state = X86_64_PROCESS_RUNNING;

    if (process->entry == (x86_64_process_entry_t)0 || process->kernel_stack_top == 0ULL) {
        process->state = X86_64_PROCESS_FAILED;
        refresh_counts();
        return 0U;
    }

    x86_64_process_enter_stack(process->kernel_stack_top, process->entry, process->arg);
    process_state.stack_switches += 1U;
    process->state = X86_64_PROCESS_EXITED;
    process->exit_code = 0U;
    process_state.run_count += 1U;
    refresh_counts();

    return 1U;
}

u32 x86_64_process_run_all_ready(u32 max_runs)
{
    u32 runs = 0U;
    while (runs < max_runs) {
        if (x86_64_process_run_next_ready() == 0U) {
            break;
        }
        runs += 1U;
    }

    return runs;
}

static void process_worker_a(void *arg)
{
    (void)arg;
    worker_a_stack_sample = read_rsp();
    worker_a_runs += 1U;
}

static void process_worker_b(void *arg)
{
    (void)arg;
    worker_b_stack_sample = read_rsp();
    worker_b_runs += 1U;
}

void x86_64_process_run_smoke(struct x86_64_process_smoke_state *state)
{
    struct x86_64_userland_foundation_state userland_state;

    x86_64_process_initialize(&process_state);

    u32 first_pid = x86_64_process_create("boot-worker-a", process_worker_a, (void *)0);
    u32 second_pid = x86_64_process_create("boot-worker-b", process_worker_b, (void *)0);
    process_state.ready_before_run = count_processes_with_state(X86_64_PROCESS_READY);

    (void)x86_64_process_run_all_ready(X86_64_PROCESS_MAX_PROCESSES);
    refresh_counts();

    x86_64_userland_foundation_init(&userland_state);
    process_state.userland_foundation_ok = userland_state.foundation_ok;

    u32 worker_a_stack_ok = is_inside_stack(process_state.worker_a_stack_sample,
                                            process_state.first_stack_base,
                                            process_state.first_stack_top);
    u32 worker_b_stack_ok = is_inside_stack(process_state.worker_b_stack_sample,
                                            process_state.second_stack_base,
                                            process_state.second_stack_top);

    process_state.stack_alignment_ok =
        ((process_state.first_stack_base != 0ULL) &&
         (process_state.second_stack_base != 0ULL) &&
         (is_aligned_u64(process_state.first_stack_base, 16ULL) != 0U) &&
         (is_aligned_u64(process_state.first_stack_top, 16ULL) != 0U) &&
         (is_aligned_u64(process_state.second_stack_base, 16ULL) != 0U) &&
         (is_aligned_u64(process_state.second_stack_top, 16ULL) != 0U)) ? 1U : 0U;

    process_state.stack_execution_ok =
        ((process_state.stack_switches == 2U) &&
         (worker_a_stack_ok != 0U) &&
         (worker_b_stack_ok != 0U)) ? 1U : 0U;

    process_state.smoke_ok =
        ((process_state.initialized != 0U) &&
         (first_pid != X86_64_PROCESS_INVALID_PID) &&
         (second_pid != X86_64_PROCESS_INVALID_PID) &&
         (process_state.created_processes == 2U) &&
         (process_state.ready_before_run == 2U) &&
         (process_state.run_count == 2U) &&
         (process_state.exited_processes == 2U) &&
         (process_state.failed_processes == 0U) &&
         (process_state.stack_allocations == 2U) &&
         (process_state.stack_alignment_ok != 0U) &&
         (process_state.stack_execution_ok != 0U) &&
         (process_state.worker_a_count == 1U) &&
         (process_state.worker_b_count == 1U) &&
         (process_state.userland_foundation_ok != 0U)) ? 1U : 0U;

    x86_64_console_write_u32(25, 0, "Process smoke ok: ", process_state.smoke_ok);

    x86_64_serial_write_line("x86_64 process scheduler smoke online");
    x86_64_serial_write_u32("Process initialized: ", process_state.initialized);
    x86_64_serial_write_u32("Process table capacity: ", process_state.table_capacity);
    x86_64_serial_write_u32("Process created: ", process_state.created_processes);
    x86_64_serial_write_u32("Process ready before run: ", process_state.ready_before_run);
    x86_64_serial_write_u32("Process run attempts: ", process_state.run_attempts);
    x86_64_serial_write_u32("Process run count: ", process_state.run_count);
    x86_64_serial_write_u32("Process exited: ", process_state.exited_processes);
    x86_64_serial_write_u32("Process failed: ", process_state.failed_processes);
    x86_64_serial_write_u32("Process stack allocations: ", process_state.stack_allocations);
    x86_64_serial_write_u32("Process stack alignment ok: ", process_state.stack_alignment_ok);
    x86_64_serial_write_u32("Process stack switches: ", process_state.stack_switches);
    x86_64_serial_write_u32("Process stack execution ok: ", process_state.stack_execution_ok);
    x86_64_serial_write_u32("Process last created pid: ", process_state.last_created_pid);
    x86_64_serial_write_u32("Process last run pid: ", process_state.last_run_pid);
    x86_64_serial_write_hex64("Process first stack base: 0x", process_state.first_stack_base);
    x86_64_serial_write_hex64("Process first stack top: 0x", process_state.first_stack_top);
    x86_64_serial_write_hex64("Process second stack base: 0x", process_state.second_stack_base);
    x86_64_serial_write_hex64("Process second stack top: 0x", process_state.second_stack_top);
    x86_64_serial_write_hex64("Process worker A stack sample: 0x", process_state.worker_a_stack_sample);
    x86_64_serial_write_hex64("Process worker B stack sample: 0x", process_state.worker_b_stack_sample);
    x86_64_serial_write_u32("Process worker A count: ", process_state.worker_a_count);
    x86_64_serial_write_u32("Process worker B count: ", process_state.worker_b_count);
    x86_64_serial_write_u32("Process userland foundation ok: ", process_state.userland_foundation_ok);
    x86_64_userland_foundation_report(&userland_state);
    x86_64_serial_write_u32("Process smoke ok: ", process_state.smoke_ok);

    if (state != &process_state) {
        *state = process_state;
    }
}

const struct x86_64_process_smoke_state *x86_64_process_get_smoke_state(void)
{
    return &process_state;
}
