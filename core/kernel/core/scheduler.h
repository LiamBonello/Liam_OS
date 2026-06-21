#ifndef LIAM_OS_SCHEDULER_H
#define LIAM_OS_SCHEDULER_H

#include "types.h"

#include "status.h"


#define SCHEDULER_MAX_TASKS 16
#define SCHEDULER_TASK_NAME_MAX 32
#define SCHEDULER_KERNEL_STACK_SIZE 4096

#define SCHEDULER_NO_TASK_SELECTED 0xFFFFFFFF

typedef enum
{
    SCHEDULER_TASK_UNUSED = 0,
    SCHEDULER_TASK_ACTIVE = 1,
    SCHEDULER_TASK_RUNNING = 2,
    SCHEDULER_TASK_DISABLED = 3,
    SCHEDULER_TASK_COMPLETED = 4,
    SCHEDULER_TASK_SLEEPING = 5
} scheduler_task_state_t;

typedef enum
{
    SCHEDULER_TASK_RECURRING = 0,
    SCHEDULER_TASK_ONESHOT = 1
} scheduler_task_mode_t;

typedef void (*scheduler_task_entry_t)(void);

typedef struct
{
    uint32_t eip;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eflags;

    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
} scheduler_context_t;

typedef struct
{
    uint8_t used;
    uint32_t id;
    char name[SCHEDULER_TASK_NAME_MAX];

    scheduler_task_state_t state;
    scheduler_task_mode_t mode;

    uint32_t interval_ticks;
    uint32_t last_run_tick;
    uint32_t run_count;

    uint32_t wake_tick;
    uint32_t sleep_count;

    void* kernel_stack_base;
    uint32_t kernel_stack_size;
    uint32_t kernel_stack_top;

    scheduler_context_t context;
    scheduler_task_entry_t entry;
} scheduler_task_t;

typedef struct
{
    uint8_t stack_allocated;
    uint8_t started;
    uint8_t entered;
    uint8_t completed;
    uint8_t returned_unexpectedly;

    uint32_t run_count;
    uint32_t test_stack_base;
    uint32_t test_stack_top;
    uint32_t test_entry_address;
    uint32_t return_eip;
    uint32_t test_saved_eip;
} scheduler_switch_test_info_t;

typedef struct
{
    uint8_t enabled;
    uint8_t in_timer_dispatch;

    uint32_t tick_interval;
    uint32_t last_dispatch_tick;
    uint32_t timer_ticks_seen;
    uint32_t timer_dispatches;
    uint32_t skipped_reentrant_dispatches;
} scheduler_preempt_test_info_t;

typedef struct
{
    uint32_t rebuilds;
    uint32_t count;
    uint32_t head;
    uint32_t tail;
    uint32_t overflows;
    uint32_t selected_slot;
    uint32_t task_ids[SCHEDULER_MAX_TASKS];
} scheduler_ready_queue_info_t;

typedef struct
{
    uint32_t audits_run;
    uint32_t repairs_run;
    uint32_t issues_found;
    uint32_t issues_repaired;

    uint32_t invalid_used_state;
    uint32_t unused_task_with_stack;
    uint32_t active_task_without_stack;
    uint32_t active_task_without_entry;
    uint32_t sleeping_task_without_wake_tick;
    uint32_t inactive_task_with_stack;

    uint32_t ready_queue_invalid_slot;
    uint32_t ready_queue_task_id_mismatch;
    uint32_t ready_queue_over_capacity;

    uint32_t current_task_invalid;
    uint32_t switched_task_invalid;
} scheduler_audit_info_t;

typedef struct
{
    uint8_t locked;
    uint8_t interrupts_enabled_now;

    uint32_t depth;
    uint32_t saved_flags;

    uint32_t lock_count;
    uint32_t unlock_count;
    uint32_t max_depth;
    uint32_t failed_unlocks;
    uint32_t test_runs;
} scheduler_lock_info_t;

typedef struct
{
    uint8_t initialized;
    uint32_t next_task_id;

    uint32_t active_tasks;
    uint32_t running_tasks;
    uint32_t completed_tasks;
    uint32_t disabled_tasks;
    uint32_t sleeping_tasks;

    uint32_t total_created_tasks;
    uint32_t scheduler_runs;
    uint32_t task_dispatches;
    uint32_t manual_dispatches;
    uint32_t switched_dispatches;
    uint32_t switched_returns;

    uint32_t current_task_index;
    uint32_t last_selected_task_id;
    uint32_t round_robin_passes;
    uint32_t tasks_considered;
    uint32_t tasks_selected;
    uint32_t no_task_selected;

    uint32_t allocated_kernel_stacks;
    uint32_t freed_kernel_stacks;
    uint32_t failed_stack_allocations;

    uint32_t trampoline_entries;
    uint32_t task_completions;
    uint32_t task_sleeps;
    uint32_t task_wakes;
    uint32_t task_yields;
    uint32_t failed_task_yields;
    uint32_t last_yielded_task_id;

    uint32_t context_self_tests;
    uint32_t switch_tests;
    uint32_t switch_test_failures;
} scheduler_stats_t;

void scheduler_initialize(void);

int scheduler_create_kernel_task(
    const char* name,
    uint32_t interval_ticks,
    scheduler_task_entry_t entry
);

int scheduler_create_oneshot_kernel_task(
    const char* name,
    scheduler_task_entry_t entry
);

kernel_status_t scheduler_create_kernel_task_status(
    const char* name,
    uint32_t interval_ticks,
    scheduler_task_entry_t entry,
    uint32_t* task_id
);

kernel_status_t scheduler_create_oneshot_kernel_task_status(
    const char* name,
    scheduler_task_entry_t entry,
    uint32_t* task_id
);

int scheduler_disable_task(uint32_t task_id);
int scheduler_sleep_task(uint32_t task_id, uint32_t sleep_ticks);
int scheduler_wake_task(uint32_t task_id);
int scheduler_yield_task(uint32_t task_id);
int scheduler_yield_current_task(void);
int scheduler_run_task_now(uint32_t task_id);
int scheduler_run_task_with_context_switch(uint32_t task_id);
uint32_t scheduler_clear_inactive_tasks(void);

kernel_status_t scheduler_disable_task_status(uint32_t task_id);
kernel_status_t scheduler_sleep_task_status(uint32_t task_id, uint32_t sleep_ticks);
kernel_status_t scheduler_wake_task_status(uint32_t task_id);
kernel_status_t scheduler_yield_task_status(uint32_t task_id);

void scheduler_run_pending(void);
void scheduler_on_timer_tick(uint32_t current_tick);

void scheduler_context_switch(
    scheduler_context_t* old_context,
    scheduler_context_t* next_context
);

int scheduler_run_context_self_test(void);
int scheduler_run_switch_test(void);
void scheduler_reset_switch_test(void);
const scheduler_switch_test_info_t* scheduler_get_switch_test_info(void);

void scheduler_enable_preempt_test(uint32_t tick_interval);
void scheduler_disable_preempt_test(void);
void scheduler_reset_preempt_test(void);
const scheduler_preempt_test_info_t* scheduler_get_preempt_test_info(void);

const scheduler_ready_queue_info_t* scheduler_get_ready_queue_info(void);

uint32_t scheduler_audit(void);
uint32_t scheduler_repair(void);
const scheduler_audit_info_t* scheduler_get_audit_info(void);

void scheduler_lock(void);
void scheduler_unlock(void);
int scheduler_run_lock_test(void);
const scheduler_lock_info_t* scheduler_get_lock_info(void);

const scheduler_stats_t* scheduler_get_stats(void);
const scheduler_task_t* scheduler_get_tasks(void);
const scheduler_task_t* scheduler_get_task(uint32_t task_id);
const scheduler_task_t* scheduler_get_current_task(void);

void scheduler_create_test_workload(void);
void scheduler_disable_test_workload(void);
void scheduler_reset_test_workload_counter(void);
uint32_t scheduler_get_test_workload_counter(void);
uint8_t scheduler_is_test_workload_active(void);

#endif
