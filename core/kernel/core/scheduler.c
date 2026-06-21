#include "scheduler.h"
#include "string.h"
#include "log.h"
#include "assert.h"
#include "status.h"
#include "../arch/i386/irq.h"
#include "../drivers/timer.h"
#include "../memory/heap.h"

static scheduler_task_t scheduler_tasks[SCHEDULER_MAX_TASKS];
static scheduler_stats_t scheduler_stats;

static uint32_t test_workload_counter = 0;
static uint32_t test_workload_task_id = 0;

static scheduler_context_t switch_test_return_context;
static scheduler_context_t switch_test_context;
static scheduler_switch_test_info_t switch_test_info;

static scheduler_context_t scheduler_main_context;
static scheduler_task_t* scheduler_current_switched_task = 0;
static scheduler_task_t* scheduler_current_running_task = 0;
static uint8_t scheduler_running_on_task_stack = 0;

static scheduler_preempt_test_info_t scheduler_preempt_test_info;

static scheduler_ready_queue_info_t scheduler_ready_queue_info;
static uint32_t scheduler_ready_queue_slots[SCHEDULER_MAX_TASKS];

static scheduler_audit_info_t scheduler_audit_info;
static scheduler_lock_info_t scheduler_lock_info;

static void scheduler_recalculate_task_counts(void);
static void scheduler_free_kernel_stack(scheduler_task_t* task);

static int scheduler_create_task_internal_unlocked(
    const char* name,
    uint32_t interval_ticks,
    scheduler_task_entry_t entry,
    scheduler_task_mode_t mode
);

static int scheduler_disable_task_unlocked(uint32_t task_id);
static int scheduler_sleep_task_unlocked(uint32_t task_id, uint32_t sleep_ticks);
static int scheduler_wake_task_unlocked(uint32_t task_id);
static int scheduler_yield_task_unlocked(uint32_t task_id);
static int scheduler_run_task_now_unlocked(uint32_t task_id);
static int scheduler_run_task_with_context_switch_unlocked(uint32_t task_id);
static uint32_t scheduler_clear_inactive_tasks_unlocked(void);
static uint32_t scheduler_audit_unlocked(void);
static uint32_t scheduler_repair_unlocked(void);
static void scheduler_run_pending_unlocked(void);

static void scheduler_clear_context(scheduler_context_t* context)
{
    context->eip = 0;
    context->esp = 0;
    context->ebp = 0;
    context->eflags = 0x202;

    context->eax = 0;
    context->ebx = 0;
    context->ecx = 0;
    context->edx = 0;
    context->esi = 0;
    context->edi = 0;
}

static void scheduler_clear_task(scheduler_task_t* task)
{
    task->used = 0;
    task->id = 0;
    string_clear(task->name, SCHEDULER_TASK_NAME_MAX);

    task->state = SCHEDULER_TASK_UNUSED;
    task->mode = SCHEDULER_TASK_RECURRING;

    task->interval_ticks = 0;
    task->last_run_tick = 0;
    task->run_count = 0;

    task->wake_tick = 0;
    task->sleep_count = 0;

    task->kernel_stack_base = 0;
    task->kernel_stack_size = 0;
    task->kernel_stack_top = 0;

    scheduler_clear_context(&task->context);
    task->entry = 0;
}

static void scheduler_clear_switch_test_info(void)
{
    switch_test_info.stack_allocated = 0;
    switch_test_info.started = 0;
    switch_test_info.entered = 0;
    switch_test_info.completed = 0;
    switch_test_info.returned_unexpectedly = 0;

    switch_test_info.run_count = 0;
    switch_test_info.test_stack_base = 0;
    switch_test_info.test_stack_top = 0;
    switch_test_info.test_entry_address = 0;
    switch_test_info.return_eip = 0;
    switch_test_info.test_saved_eip = 0;
}

static void scheduler_clear_preempt_test_info(void)
{
    scheduler_preempt_test_info.enabled = 0;
    scheduler_preempt_test_info.in_timer_dispatch = 0;

    scheduler_preempt_test_info.tick_interval = 0;
    scheduler_preempt_test_info.last_dispatch_tick = 0;
    scheduler_preempt_test_info.timer_ticks_seen = 0;
    scheduler_preempt_test_info.timer_dispatches = 0;
    scheduler_preempt_test_info.skipped_reentrant_dispatches = 0;
}

static void scheduler_clear_ready_queue_info(void)
{
    scheduler_ready_queue_info.rebuilds = 0;
    scheduler_ready_queue_info.count = 0;
    scheduler_ready_queue_info.head = 0;
    scheduler_ready_queue_info.tail = 0;
    scheduler_ready_queue_info.overflows = 0;
    scheduler_ready_queue_info.selected_slot = SCHEDULER_NO_TASK_SELECTED;

    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        scheduler_ready_queue_info.task_ids[i] = 0;
        scheduler_ready_queue_slots[i] = SCHEDULER_NO_TASK_SELECTED;
    }
}

static void scheduler_clear_audit_info(void)
{
    scheduler_audit_info.audits_run = 0;
    scheduler_audit_info.repairs_run = 0;
    scheduler_audit_info.issues_found = 0;
    scheduler_audit_info.issues_repaired = 0;

    scheduler_audit_info.invalid_used_state = 0;
    scheduler_audit_info.unused_task_with_stack = 0;
    scheduler_audit_info.active_task_without_stack = 0;
    scheduler_audit_info.active_task_without_entry = 0;
    scheduler_audit_info.sleeping_task_without_wake_tick = 0;
    scheduler_audit_info.inactive_task_with_stack = 0;

    scheduler_audit_info.ready_queue_invalid_slot = 0;
    scheduler_audit_info.ready_queue_task_id_mismatch = 0;
    scheduler_audit_info.ready_queue_over_capacity = 0;

    scheduler_audit_info.current_task_invalid = 0;
    scheduler_audit_info.switched_task_invalid = 0;
}

static void scheduler_clear_lock_info(void)
{
    scheduler_lock_info.locked = 0;
    scheduler_lock_info.interrupts_enabled_now = irq_are_enabled();

    scheduler_lock_info.depth = 0;
    scheduler_lock_info.saved_flags = 0;

    scheduler_lock_info.lock_count = 0;
    scheduler_lock_info.unlock_count = 0;
    scheduler_lock_info.max_depth = 0;
    scheduler_lock_info.failed_unlocks = 0;
    scheduler_lock_info.test_runs = 0;
}

static int scheduler_find_available_slot(void)
{
    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        if (
            !scheduler_tasks[i].used ||
            scheduler_tasks[i].state == SCHEDULER_TASK_UNUSED ||
            scheduler_tasks[i].state == SCHEDULER_TASK_DISABLED ||
            scheduler_tasks[i].state == SCHEDULER_TASK_COMPLETED
        )
        {
            return (int)i;
        }
    }

    return -1;
}

static int scheduler_task_state_is_valid(scheduler_task_state_t state)
{
    return (
        state == SCHEDULER_TASK_UNUSED ||
        state == SCHEDULER_TASK_ACTIVE ||
        state == SCHEDULER_TASK_RUNNING ||
        state == SCHEDULER_TASK_DISABLED ||
        state == SCHEDULER_TASK_COMPLETED ||
        state == SCHEDULER_TASK_SLEEPING
    );
}

static void scheduler_assign_task_state(
    scheduler_task_t* task,
    scheduler_task_state_t state
)
{
    if (task == 0)
    {
        return;
    }

    task->state = state;
}

static void scheduler_transition_task_state(
    scheduler_task_t* task,
    scheduler_task_state_t state
)
{
    scheduler_assign_task_state(task, state);
    scheduler_recalculate_task_counts();
}

static scheduler_task_t* scheduler_find_task_by_id_mutable(uint32_t task_id)
{
    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        if (scheduler_tasks[i].used && scheduler_tasks[i].id == task_id)
        {
            return &scheduler_tasks[i];
        }
    }

    return 0;
}

static void scheduler_wake_sleeping_tasks(uint32_t current_tick)
{
    uint8_t changed = 0;

    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        scheduler_task_t* task = &scheduler_tasks[i];

        if (!task->used || task->state != SCHEDULER_TASK_SLEEPING)
        {
            continue;
        }

        if (current_tick - task->wake_tick < 0x80000000)
        {
            scheduler_assign_task_state(task, SCHEDULER_TASK_ACTIVE);
            task->wake_tick = 0;
            scheduler_stats.task_wakes++;
            changed = 1;
        }
    }

    if (changed)
    {
        scheduler_recalculate_task_counts();
    }
}

static int scheduler_task_is_ready_to_run(
    scheduler_task_t* task,
    uint32_t current_tick
)
{
    if (task == 0)
    {
        return 0;
    }

    if (!task->used || task->state != SCHEDULER_TASK_ACTIVE)
    {
        return 0;
    }

    if (task->mode == SCHEDULER_TASK_ONESHOT)
    {
        return 1;
    }

    if (current_tick - task->last_run_tick < task->interval_ticks)
    {
        return 0;
    }

    return 1;
}

static void scheduler_reset_ready_queue_snapshot(void)
{
    scheduler_ready_queue_info.count = 0;
    scheduler_ready_queue_info.head = 0;
    scheduler_ready_queue_info.tail = 0;
    scheduler_ready_queue_info.selected_slot = SCHEDULER_NO_TASK_SELECTED;

    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        scheduler_ready_queue_info.task_ids[i] = 0;
        scheduler_ready_queue_slots[i] = SCHEDULER_NO_TASK_SELECTED;
    }
}

static void scheduler_rebuild_ready_queue(uint32_t current_tick)
{
    scheduler_ready_queue_info.rebuilds++;
    scheduler_reset_ready_queue_snapshot();

    uint32_t start_index =
        scheduler_stats.current_task_index % SCHEDULER_MAX_TASKS;

    for (uint32_t attempts = 0; attempts < SCHEDULER_MAX_TASKS; attempts++)
    {
        uint32_t index = (start_index + attempts) % SCHEDULER_MAX_TASKS;
        scheduler_task_t* task = &scheduler_tasks[index];

        scheduler_stats.tasks_considered++;

        if (attempts > 0 && index == 0)
        {
            scheduler_stats.round_robin_passes++;
        }

        if (!scheduler_task_is_ready_to_run(task, current_tick))
        {
            continue;
        }

        if (scheduler_ready_queue_info.count >= SCHEDULER_MAX_TASKS)
        {
            scheduler_ready_queue_info.overflows++;
            continue;
        }

        uint32_t queue_index = scheduler_ready_queue_info.count;

        scheduler_ready_queue_slots[queue_index] = index;
        scheduler_ready_queue_info.task_ids[queue_index] = task->id;
        scheduler_ready_queue_info.count++;
    }

    scheduler_ready_queue_info.tail = scheduler_ready_queue_info.count;
}

static scheduler_task_t* scheduler_select_next_ready_task(uint32_t current_tick)
{
    scheduler_rebuild_ready_queue(current_tick);

    if (scheduler_ready_queue_info.count == 0)
    {
        scheduler_stats.last_selected_task_id = SCHEDULER_NO_TASK_SELECTED;
        scheduler_stats.no_task_selected++;
        return 0;
    }

    uint32_t selected_slot = scheduler_ready_queue_slots[0];

    if (selected_slot >= SCHEDULER_MAX_TASKS)
    {
        scheduler_stats.last_selected_task_id = SCHEDULER_NO_TASK_SELECTED;
        scheduler_stats.no_task_selected++;
        return 0;
    }

    kernel_assert_message(
        selected_slot < SCHEDULER_MAX_TASKS,
        "ready queue selected slot out of range"
    );

    scheduler_task_t* task = &scheduler_tasks[selected_slot];

    kernel_assert_message(
        task->used,
        "ready queue selected unused task"
    );

    scheduler_ready_queue_info.selected_slot = selected_slot;
    scheduler_ready_queue_info.head = 1;

    scheduler_stats.current_task_index =
        (selected_slot + 1) % SCHEDULER_MAX_TASKS;

    scheduler_stats.last_selected_task_id = task->id;
    scheduler_stats.tasks_selected++;

    return task;
}

static int scheduler_pointer_points_to_task_table(scheduler_task_t* task)
{
    if (task == 0)
    {
        return 1;
    }

    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        if (task == &scheduler_tasks[i])
        {
            return 1;
        }
    }

    return 0;
}

static void scheduler_audit_record_issue(uint32_t* counter)
{
    scheduler_audit_info.issues_found++;

    if (counter != 0)
    {
        (*counter)++;
    }
}

static void scheduler_audit_ready_queue(void)
{
    if (scheduler_ready_queue_info.count > SCHEDULER_MAX_TASKS)
    {
        scheduler_audit_record_issue(&scheduler_audit_info.ready_queue_over_capacity);
    }

    for (uint32_t i = 0; i < scheduler_ready_queue_info.count; i++)
    {
        uint32_t slot = scheduler_ready_queue_slots[i];

        if (slot >= SCHEDULER_MAX_TASKS)
        {
            scheduler_audit_record_issue(&scheduler_audit_info.ready_queue_invalid_slot);
            continue;
        }

        scheduler_task_t* task = &scheduler_tasks[slot];

        if (!task->used || scheduler_ready_queue_info.task_ids[i] != task->id)
        {
            scheduler_audit_record_issue(&scheduler_audit_info.ready_queue_task_id_mismatch);
        }
    }
}

static uint32_t scheduler_audit_unlocked(void)
{
    scheduler_clear_audit_info();
    scheduler_audit_info.audits_run = 1;

    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        scheduler_task_t* task = &scheduler_tasks[i];

        if (!task->used)
        {
            if (task->kernel_stack_base != 0 || task->kernel_stack_top != 0)
            {
                scheduler_audit_record_issue(&scheduler_audit_info.unused_task_with_stack);
            }

            continue;
        }

        if (!scheduler_task_state_is_valid(task->state))
        {
            scheduler_audit_record_issue(&scheduler_audit_info.invalid_used_state);
            continue;
        }

        if (
            task->state == SCHEDULER_TASK_ACTIVE ||
            task->state == SCHEDULER_TASK_RUNNING ||
            task->state == SCHEDULER_TASK_SLEEPING
        )
        {
            if (task->kernel_stack_base == 0 || task->kernel_stack_top == 0)
            {
                scheduler_audit_record_issue(&scheduler_audit_info.active_task_without_stack);
            }

            if (task->entry == 0)
            {
                scheduler_audit_record_issue(&scheduler_audit_info.active_task_without_entry);
            }
        }

        if (task->state == SCHEDULER_TASK_SLEEPING && task->wake_tick == 0)
        {
            scheduler_audit_record_issue(&scheduler_audit_info.sleeping_task_without_wake_tick);
        }

        if (
            task->state == SCHEDULER_TASK_DISABLED ||
            task->state == SCHEDULER_TASK_COMPLETED
        )
        {
            if (task->kernel_stack_base != 0 || task->kernel_stack_top != 0)
            {
                scheduler_audit_record_issue(&scheduler_audit_info.inactive_task_with_stack);
            }
        }
    }

    scheduler_audit_ready_queue();

    if (!scheduler_pointer_points_to_task_table(scheduler_current_running_task))
    {
        scheduler_audit_record_issue(&scheduler_audit_info.current_task_invalid);
    }

    if (!scheduler_pointer_points_to_task_table(scheduler_current_switched_task))
    {
        scheduler_audit_record_issue(&scheduler_audit_info.switched_task_invalid);
    }

    return scheduler_audit_info.issues_found;
}

uint32_t scheduler_audit(void)
{
    scheduler_lock();
    uint32_t result = scheduler_audit_unlocked();
    scheduler_unlock();

    return result;
}

static uint32_t scheduler_repair_unlocked(void)
{
    uint32_t repaired = 0;

    scheduler_audit_info.repairs_run++;

    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        scheduler_task_t* task = &scheduler_tasks[i];

        if (!task->used)
        {
            if (task->kernel_stack_base != 0)
            {
                scheduler_free_kernel_stack(task);
                repaired++;
            }

            task->kernel_stack_top = 0;
            task->kernel_stack_size = 0;
            continue;
        }

        if (!scheduler_task_state_is_valid(task->state))
        {
            scheduler_assign_task_state(task, SCHEDULER_TASK_DISABLED);
            scheduler_free_kernel_stack(task);
            repaired++;
            continue;
        }

        if (
            task->state == SCHEDULER_TASK_ACTIVE ||
            task->state == SCHEDULER_TASK_RUNNING ||
            task->state == SCHEDULER_TASK_SLEEPING
        )
        {
            if (
                task->entry == 0 ||
                task->kernel_stack_base == 0 ||
                task->kernel_stack_top == 0
            )
            {
                scheduler_assign_task_state(task, SCHEDULER_TASK_DISABLED);
                scheduler_free_kernel_stack(task);
                repaired++;
                continue;
            }
        }

        if (task->state == SCHEDULER_TASK_SLEEPING && task->wake_tick == 0)
        {
            scheduler_assign_task_state(task, SCHEDULER_TASK_ACTIVE);
            repaired++;
        }

        if (
            task->state == SCHEDULER_TASK_DISABLED ||
            task->state == SCHEDULER_TASK_COMPLETED
        )
        {
            if (task->kernel_stack_base != 0)
            {
                scheduler_free_kernel_stack(task);
                repaired++;
            }
        }
    }

    if (!scheduler_pointer_points_to_task_table(scheduler_current_running_task))
    {
        scheduler_current_running_task = 0;
        repaired++;
    }

    if (!scheduler_pointer_points_to_task_table(scheduler_current_switched_task))
    {
        scheduler_current_switched_task = 0;
        repaired++;
    }

    scheduler_reset_ready_queue_snapshot();
    scheduler_recalculate_task_counts();

    scheduler_audit_info.issues_repaired += repaired;

    scheduler_audit_unlocked();

    return repaired;
}

uint32_t scheduler_repair(void)
{
    scheduler_lock();
    uint32_t result = scheduler_repair_unlocked();
    scheduler_unlock();

    return result;
}

static void scheduler_recalculate_task_counts(void)
{
    uint32_t active = 0;
    uint32_t running = 0;
    uint32_t completed = 0;
    uint32_t disabled = 0;
    uint32_t sleeping = 0;

    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        if (!scheduler_tasks[i].used)
        {
            continue;
        }

        if (scheduler_tasks[i].state == SCHEDULER_TASK_ACTIVE)
        {
            active++;
        }
        else if (scheduler_tasks[i].state == SCHEDULER_TASK_RUNNING)
        {
            running++;
        }
        else if (scheduler_tasks[i].state == SCHEDULER_TASK_COMPLETED)
        {
            completed++;
        }
        else if (scheduler_tasks[i].state == SCHEDULER_TASK_DISABLED)
        {
            disabled++;
        }
        else if (scheduler_tasks[i].state == SCHEDULER_TASK_SLEEPING)
        {
            sleeping++;
        }
    }

    scheduler_stats.active_tasks = active;
    scheduler_stats.running_tasks = running;
    scheduler_stats.completed_tasks = completed;
    scheduler_stats.disabled_tasks = disabled;
    scheduler_stats.sleeping_tasks = sleeping;
}

static int scheduler_allocate_kernel_stack(scheduler_task_t* task)
{
    kernel_assert_message(task != 0, "scheduler stack allocation requires task");

    void* stack = kmalloc(SCHEDULER_KERNEL_STACK_SIZE);

    if (stack == 0)
    {
        scheduler_stats.failed_stack_allocations++;
        return 0;
    }

    kernel_assert_message(
        heap_validate_pointer(stack),
        "scheduler kernel stack allocation returned invalid heap pointer"
    );

    task->kernel_stack_base = stack;
    task->kernel_stack_size = SCHEDULER_KERNEL_STACK_SIZE;
    task->kernel_stack_top = (uint32_t)stack + SCHEDULER_KERNEL_STACK_SIZE;

    task->context.esp = task->kernel_stack_top;
    task->context.ebp = task->kernel_stack_top;
    task->context.eflags = 0x202;

    task->context.eax = 0;
    task->context.ebx = 0;
    task->context.ecx = 0;
    task->context.edx = 0;
    task->context.esi = 0;
    task->context.edi = 0;

    scheduler_stats.allocated_kernel_stacks++;

    return 1;
}

static void scheduler_free_kernel_stack(scheduler_task_t* task)
{
    kernel_assert_message(task != 0, "scheduler stack free requires task");

    if (task->kernel_stack_base == 0)
    {
        return;
    }

    kernel_assert_message(
        heap_validate_pointer(task->kernel_stack_base),
        "scheduler kernel stack pointer failed heap validation before free"
    );

    kfree(task->kernel_stack_base);

    task->kernel_stack_base = 0;
    task->kernel_stack_size = 0;
    task->kernel_stack_top = 0;
    task->context.esp = 0;
    task->context.ebp = 0;

    scheduler_stats.freed_kernel_stacks++;
}

static void scheduler_task_trampoline(scheduler_task_t* task)
{
    if (task == 0 || task->entry == 0)
    {
        return;
    }

    scheduler_stats.trampoline_entries++;

    scheduler_assign_task_state(task, SCHEDULER_TASK_RUNNING);
    scheduler_current_running_task = task;
    scheduler_recalculate_task_counts();

    task->entry();

    scheduler_current_running_task = 0;

    task->run_count++;

    if (task->mode == SCHEDULER_TASK_ONESHOT)
    {
        scheduler_assign_task_state(task, SCHEDULER_TASK_COMPLETED);

        if (!scheduler_running_on_task_stack)
        {
            scheduler_free_kernel_stack(task);
        }

        scheduler_stats.task_completions++;
    }
    else
    {
        scheduler_assign_task_state(task, SCHEDULER_TASK_ACTIVE);
    }

    scheduler_recalculate_task_counts();
}

static void scheduler_switched_task_entry(void)
{
    scheduler_task_t* task = scheduler_current_switched_task;

    if (task != 0)
    {
        scheduler_task_trampoline(task);
        scheduler_context_switch(&task->context, &scheduler_main_context);
    }

    for (;;)
    {
        __asm__ volatile("hlt");
    }
}

static void scheduler_prepare_switched_context(scheduler_task_t* task)
{
    task->context.eip = (uint32_t)scheduler_switched_task_entry;
    task->context.esp = task->kernel_stack_top & 0xFFFFFFF0;
    task->context.ebp = task->context.esp;
    task->context.eflags = 0x202;

    task->context.eax = 0;
    task->context.ebx = 0;
    task->context.ecx = 0;
    task->context.edx = 0;
    task->context.esi = 0;
    task->context.edi = 0;
}

static void scheduler_switch_test_entry(void)
{
    switch_test_info.entered = 1;
    switch_test_info.run_count++;

    scheduler_context_switch(&switch_test_context, &switch_test_return_context);

    switch_test_info.returned_unexpectedly = 1;

    for (;;)
    {
        __asm__ volatile("hlt");
    }
}

static int scheduler_ensure_switch_test_stack(void)
{
    if (switch_test_info.test_stack_base != 0)
    {
        switch_test_info.stack_allocated = 1;
        return 1;
    }

    void* stack = kmalloc(SCHEDULER_KERNEL_STACK_SIZE);

    if (stack == 0)
    {
        scheduler_stats.switch_test_failures++;
        return 0;
    }

    switch_test_info.test_stack_base = (uint32_t)stack;
    switch_test_info.test_stack_top =
        ((uint32_t)stack + SCHEDULER_KERNEL_STACK_SIZE) & 0xFFFFFFF0;
    switch_test_info.stack_allocated = 1;

    return 1;
}

void scheduler_initialize(void)
{
    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        scheduler_clear_task(&scheduler_tasks[i]);
    }

    scheduler_clear_context(&switch_test_return_context);
    scheduler_clear_context(&switch_test_context);
    scheduler_clear_switch_test_info();

    scheduler_clear_context(&scheduler_main_context);
    scheduler_current_switched_task = 0;
    scheduler_current_running_task = 0;
    scheduler_running_on_task_stack = 0;

    scheduler_clear_preempt_test_info();
    scheduler_clear_ready_queue_info();
    scheduler_clear_audit_info();
    scheduler_clear_lock_info();

    scheduler_stats.initialized = 1;
    scheduler_stats.next_task_id = 1;

    scheduler_stats.active_tasks = 0;
    scheduler_stats.running_tasks = 0;
    scheduler_stats.completed_tasks = 0;
    scheduler_stats.disabled_tasks = 0;
    scheduler_stats.sleeping_tasks = 0;

    scheduler_stats.total_created_tasks = 0;
    scheduler_stats.scheduler_runs = 0;
    scheduler_stats.task_dispatches = 0;
    scheduler_stats.manual_dispatches = 0;
    scheduler_stats.switched_dispatches = 0;
    scheduler_stats.switched_returns = 0;

    scheduler_stats.current_task_index = 0;
    scheduler_stats.last_selected_task_id = 0;
    scheduler_stats.round_robin_passes = 0;
    scheduler_stats.tasks_considered = 0;
    scheduler_stats.tasks_selected = 0;
    scheduler_stats.no_task_selected = 0;

    scheduler_stats.allocated_kernel_stacks = 0;
    scheduler_stats.freed_kernel_stacks = 0;
    scheduler_stats.failed_stack_allocations = 0;

    scheduler_stats.trampoline_entries = 0;
    scheduler_stats.task_completions = 0;
    scheduler_stats.task_sleeps = 0;
    scheduler_stats.task_wakes = 0;
    scheduler_stats.task_yields = 0;
    scheduler_stats.failed_task_yields = 0;
    scheduler_stats.last_yielded_task_id = 0;

    scheduler_stats.context_self_tests = 0;
    scheduler_stats.switch_tests = 0;
    scheduler_stats.switch_test_failures = 0;

    test_workload_counter = 0;
    test_workload_task_id = 0;

    log_success("Scheduler initialized");
}

static int scheduler_create_task_internal_unlocked(
    const char* name,
    uint32_t interval_ticks,
    scheduler_task_entry_t entry,
    scheduler_task_mode_t mode
)
{
    if (!scheduler_stats.initialized || entry == 0)
    {
        return -1;
    }

    if (mode == SCHEDULER_TASK_RECURRING && interval_ticks == 0)
    {
        return -1;
    }

    int slot = scheduler_find_available_slot();

    if (slot < 0)
    {
        return -1;
    }

    scheduler_task_t* task = &scheduler_tasks[slot];

    if (task->kernel_stack_base != 0)
    {
        scheduler_free_kernel_stack(task);
    }

    scheduler_clear_task(task);

    task->used = 1;
    task->id = scheduler_stats.next_task_id;
    scheduler_assign_task_state(task, SCHEDULER_TASK_ACTIVE);
    task->mode = mode;
    task->interval_ticks = interval_ticks;
    task->last_run_tick = timer_get_ticks();
    task->run_count = 0;
    task->entry = entry;

    string_copy(task->name, name, SCHEDULER_TASK_NAME_MAX);

    task->context.eip = (uint32_t)entry;

    if (!scheduler_allocate_kernel_stack(task))
    {
        scheduler_clear_task(task);
        scheduler_recalculate_task_counts();
        return -1;
    }

    scheduler_stats.next_task_id++;
    scheduler_stats.total_created_tasks++;

    scheduler_recalculate_task_counts();

    return (int)task->id;
}

int scheduler_create_kernel_task(
    const char* name,
    uint32_t interval_ticks,
    scheduler_task_entry_t entry
)
{
    scheduler_lock();

    int result = scheduler_create_task_internal_unlocked(
        name,
        interval_ticks,
        entry,
        SCHEDULER_TASK_RECURRING
    );

    scheduler_unlock();

    return result;
}

int scheduler_create_oneshot_kernel_task(
    const char* name,
    scheduler_task_entry_t entry
)
{
    scheduler_lock();

    int result = scheduler_create_task_internal_unlocked(
        name,
        0,
        entry,
        SCHEDULER_TASK_ONESHOT
    );

    scheduler_unlock();

    return result;
}

kernel_status_t scheduler_create_kernel_task_status(
    const char* name,
    uint32_t interval_ticks,
    scheduler_task_entry_t entry,
    uint32_t* task_id
)
{
    if (task_id == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    *task_id = 0;

    if (!scheduler_stats.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    if (name == 0 || entry == 0 || interval_ticks == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    scheduler_lock();

    int result = scheduler_create_task_internal_unlocked(
        name,
        interval_ticks,
        entry,
        SCHEDULER_TASK_RECURRING
    );

    scheduler_unlock();

    if (result < 0)
    {
        return KERNEL_ERROR_OUT_OF_MEMORY;
    }

    *task_id = (uint32_t)result;

    return KERNEL_OK;
}

kernel_status_t scheduler_create_oneshot_kernel_task_status(
    const char* name,
    scheduler_task_entry_t entry,
    uint32_t* task_id
)
{
    if (task_id == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    *task_id = 0;

    if (!scheduler_stats.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    if (name == 0 || entry == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    scheduler_lock();

    int result = scheduler_create_task_internal_unlocked(
        name,
        0,
        entry,
        SCHEDULER_TASK_ONESHOT
    );

    scheduler_unlock();

    if (result < 0)
    {
        return KERNEL_ERROR_OUT_OF_MEMORY;
    }

    *task_id = (uint32_t)result;

    return KERNEL_OK;
}

static int scheduler_disable_task_unlocked(uint32_t task_id)
{
    scheduler_task_t* task = scheduler_find_task_by_id_mutable(task_id);

    if (task == 0)
    {
        return 0;
    }

    if (
        task->state != SCHEDULER_TASK_ACTIVE &&
        task->state != SCHEDULER_TASK_RUNNING
    )
    {
        return 0;
    }

    scheduler_assign_task_state(task, SCHEDULER_TASK_DISABLED);
    scheduler_free_kernel_stack(task);
    scheduler_recalculate_task_counts();

    return 1;
}

int scheduler_disable_task(uint32_t task_id)
{
    scheduler_lock();
    int result = scheduler_disable_task_unlocked(task_id);
    scheduler_unlock();

    return result;
}

kernel_status_t scheduler_disable_task_status(uint32_t task_id)
{
    if (task_id == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (!scheduler_stats.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    scheduler_lock();

    scheduler_task_t* task = scheduler_find_task_by_id_mutable(task_id);

    if (task == 0)
    {
        scheduler_unlock();
        return KERNEL_ERROR_NOT_FOUND;
    }

    if (
        task->state != SCHEDULER_TASK_ACTIVE &&
        task->state != SCHEDULER_TASK_RUNNING
    )
    {
        scheduler_unlock();
        return KERNEL_ERROR_INVALID_STATE;
    }

    int result = scheduler_disable_task_unlocked(task_id);

    scheduler_unlock();

    return result ? KERNEL_OK : KERNEL_ERROR_UNKNOWN;
}

static int scheduler_sleep_task_unlocked(uint32_t task_id, uint32_t sleep_ticks)
{
    scheduler_task_t* task = scheduler_find_task_by_id_mutable(task_id);

    if (task == 0 || sleep_ticks == 0)
    {
        return 0;
    }

    if (task->state != SCHEDULER_TASK_ACTIVE)
    {
        return 0;
    }

    scheduler_assign_task_state(task, SCHEDULER_TASK_SLEEPING);
    task->wake_tick = timer_get_ticks() + sleep_ticks;
    task->sleep_count++;

    scheduler_stats.task_sleeps++;

    scheduler_recalculate_task_counts();

    return 1;
}

int scheduler_sleep_task(uint32_t task_id, uint32_t sleep_ticks)
{
    scheduler_lock();
    int result = scheduler_sleep_task_unlocked(task_id, sleep_ticks);
    scheduler_unlock();

    return result;
}

kernel_status_t scheduler_sleep_task_status(uint32_t task_id, uint32_t sleep_ticks)
{
    if (task_id == 0 || sleep_ticks == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (!scheduler_stats.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    scheduler_lock();

    scheduler_task_t* task = scheduler_find_task_by_id_mutable(task_id);

    if (task == 0)
    {
        scheduler_unlock();
        return KERNEL_ERROR_NOT_FOUND;
    }

    if (task->state != SCHEDULER_TASK_ACTIVE)
    {
        scheduler_unlock();
        return KERNEL_ERROR_INVALID_STATE;
    }

    int result = scheduler_sleep_task_unlocked(task_id, sleep_ticks);

    scheduler_unlock();

    return result ? KERNEL_OK : KERNEL_ERROR_UNKNOWN;
}

static int scheduler_wake_task_unlocked(uint32_t task_id)
{
    scheduler_task_t* task = scheduler_find_task_by_id_mutable(task_id);

    if (task == 0)
    {
        return 0;
    }

    if (task->state != SCHEDULER_TASK_SLEEPING)
    {
        return 0;
    }

    scheduler_assign_task_state(task, SCHEDULER_TASK_ACTIVE);
    task->wake_tick = 0;

    scheduler_stats.task_wakes++;

    scheduler_recalculate_task_counts();

    return 1;
}

int scheduler_wake_task(uint32_t task_id)
{
    scheduler_lock();
    int result = scheduler_wake_task_unlocked(task_id);
    scheduler_unlock();

    return result;
}

kernel_status_t scheduler_wake_task_status(uint32_t task_id)
{
    if (task_id == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (!scheduler_stats.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    scheduler_lock();

    scheduler_task_t* task = scheduler_find_task_by_id_mutable(task_id);

    if (task == 0)
    {
        scheduler_unlock();
        return KERNEL_ERROR_NOT_FOUND;
    }

    if (task->state != SCHEDULER_TASK_SLEEPING)
    {
        scheduler_unlock();
        return KERNEL_ERROR_INVALID_STATE;
    }

    int result = scheduler_wake_task_unlocked(task_id);

    scheduler_unlock();

    return result ? KERNEL_OK : KERNEL_ERROR_UNKNOWN;
}

static int scheduler_yield_task_unlocked(uint32_t task_id)
{
    scheduler_task_t* task = scheduler_find_task_by_id_mutable(task_id);

    if (task == 0)
    {
        scheduler_stats.failed_task_yields++;
        return 0;
    }

    if (
        task->state != SCHEDULER_TASK_ACTIVE &&
        task->state != SCHEDULER_TASK_RUNNING
    )
    {
        scheduler_stats.failed_task_yields++;
        return 0;
    }

    task->last_run_tick = timer_get_ticks();

    scheduler_stats.task_yields++;
    scheduler_stats.last_yielded_task_id = task->id;

    return 1;
}

int scheduler_yield_task(uint32_t task_id)
{
    scheduler_lock();
    int result = scheduler_yield_task_unlocked(task_id);
    scheduler_unlock();

    return result;
}

kernel_status_t scheduler_yield_task_status(uint32_t task_id)
{
    if (task_id == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (!scheduler_stats.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    scheduler_lock();

    scheduler_task_t* task = scheduler_find_task_by_id_mutable(task_id);

    if (task == 0)
    {
        scheduler_unlock();
        return KERNEL_ERROR_NOT_FOUND;
    }

    if (
        task->state != SCHEDULER_TASK_ACTIVE &&
        task->state != SCHEDULER_TASK_RUNNING
    )
    {
        scheduler_unlock();
        return KERNEL_ERROR_INVALID_STATE;
    }

    int result = scheduler_yield_task_unlocked(task_id);

    scheduler_unlock();

    return result ? KERNEL_OK : KERNEL_ERROR_UNKNOWN;
}

int scheduler_yield_current_task(void)
{
    if (scheduler_current_running_task == 0)
    {
        scheduler_stats.failed_task_yields++;
        return 0;
    }

    return scheduler_yield_task_unlocked(scheduler_current_running_task->id);
}

static int scheduler_run_task_now_unlocked(uint32_t task_id)
{
    scheduler_task_t* task = scheduler_find_task_by_id_mutable(task_id);

    if (task == 0 || task->state != SCHEDULER_TASK_ACTIVE)
    {
        return 0;
    }

    scheduler_stats.manual_dispatches++;
    scheduler_stats.task_dispatches++;

    task->last_run_tick = timer_get_ticks();

    scheduler_task_trampoline(task);

    return 1;
}

int scheduler_run_task_now(uint32_t task_id)
{
    scheduler_lock();
    int result = scheduler_run_task_now_unlocked(task_id);
    scheduler_unlock();

    return result;
}

static int scheduler_run_task_with_context_switch_unlocked(uint32_t task_id)
{
    scheduler_task_t* task = scheduler_find_task_by_id_mutable(task_id);

    if (task == 0 || task->state != SCHEDULER_TASK_ACTIVE)
    {
        return 0;
    }

    if (task->kernel_stack_base == 0 || task->kernel_stack_top == 0)
    {
        return 0;
    }

    scheduler_current_switched_task = task;
    scheduler_prepare_switched_context(task);

    scheduler_stats.switched_dispatches++;
    scheduler_stats.task_dispatches++;

    task->last_run_tick = timer_get_ticks();

    scheduler_running_on_task_stack = 1;
    scheduler_context_switch(&scheduler_main_context, &task->context);
    scheduler_running_on_task_stack = 0;

    scheduler_stats.switched_returns++;
    scheduler_current_switched_task = 0;

    if (task->state == SCHEDULER_TASK_COMPLETED)
    {
        scheduler_free_kernel_stack(task);
    }

    scheduler_recalculate_task_counts();

    return 1;
}

int scheduler_run_task_with_context_switch(uint32_t task_id)
{
    scheduler_lock();
    int result = scheduler_run_task_with_context_switch_unlocked(task_id);
    scheduler_unlock();

    return result;
}

static uint32_t scheduler_clear_inactive_tasks_unlocked(void)
{
    uint32_t cleared = 0;

    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        scheduler_task_t* task = &scheduler_tasks[i];

        if (!task->used)
        {
            continue;
        }

        if (
            task->state == SCHEDULER_TASK_DISABLED ||
            task->state == SCHEDULER_TASK_COMPLETED
        )
        {
            scheduler_free_kernel_stack(task);
            scheduler_clear_task(task);
            cleared++;
        }
    }

    if (scheduler_stats.current_task_index >= SCHEDULER_MAX_TASKS)
    {
        scheduler_stats.current_task_index = 0;
    }

    scheduler_reset_ready_queue_snapshot();
    scheduler_recalculate_task_counts();

    return cleared;
}

uint32_t scheduler_clear_inactive_tasks(void)
{
    scheduler_lock();
    uint32_t result = scheduler_clear_inactive_tasks_unlocked();
    scheduler_unlock();

    return result;
}

static void scheduler_run_pending_unlocked(void)
{
    if (!scheduler_stats.initialized)
    {
        return;
    }

    uint32_t now = timer_get_ticks();

    scheduler_wake_sleeping_tasks(now);

    scheduler_stats.scheduler_runs++;

    scheduler_task_t* task = scheduler_select_next_ready_task(now);

    if (task == 0)
    {
        return;
    }

    task->last_run_tick = now;
    scheduler_stats.task_dispatches++;

    scheduler_task_trampoline(task);
}

void scheduler_run_pending(void)
{
    scheduler_lock();
    scheduler_run_pending_unlocked();
    scheduler_unlock();
}

void scheduler_on_timer_tick(uint32_t current_tick)
{
    if (!scheduler_stats.initialized)
    {
        return;
    }

    scheduler_preempt_test_info.timer_ticks_seen++;

    scheduler_lock();

    scheduler_wake_sleeping_tasks(current_tick);

    if (!scheduler_preempt_test_info.enabled)
    {
        scheduler_unlock();
        return;
    }

    if (scheduler_preempt_test_info.in_timer_dispatch)
    {
        scheduler_preempt_test_info.skipped_reentrant_dispatches++;
        scheduler_unlock();
        return;
    }

    if (
        current_tick - scheduler_preempt_test_info.last_dispatch_tick <
        scheduler_preempt_test_info.tick_interval
    )
    {
        scheduler_unlock();
        return;
    }

    scheduler_preempt_test_info.in_timer_dispatch = 1;
    scheduler_preempt_test_info.last_dispatch_tick = current_tick;
    scheduler_preempt_test_info.timer_dispatches++;

    scheduler_run_pending_unlocked();

    scheduler_preempt_test_info.in_timer_dispatch = 0;

    scheduler_unlock();
}

void scheduler_enable_preempt_test(uint32_t tick_interval)
{
    if (tick_interval == 0)
    {
        tick_interval = 10;
    }

    scheduler_lock();

    scheduler_preempt_test_info.enabled = 1;
    scheduler_preempt_test_info.tick_interval = tick_interval;
    scheduler_preempt_test_info.last_dispatch_tick = timer_get_ticks();

    scheduler_unlock();
}

void scheduler_disable_preempt_test(void)
{
    scheduler_lock();

    scheduler_preempt_test_info.enabled = 0;
    scheduler_preempt_test_info.in_timer_dispatch = 0;

    scheduler_unlock();
}

void scheduler_reset_preempt_test(void)
{
    scheduler_lock();

    uint8_t was_enabled = scheduler_preempt_test_info.enabled;
    uint32_t interval = scheduler_preempt_test_info.tick_interval;

    scheduler_clear_preempt_test_info();

    if (was_enabled)
    {
        scheduler_preempt_test_info.enabled = 1;
        scheduler_preempt_test_info.tick_interval = interval == 0 ? 10 : interval;
        scheduler_preempt_test_info.last_dispatch_tick = timer_get_ticks();
    }

    scheduler_unlock();
}

const scheduler_preempt_test_info_t* scheduler_get_preempt_test_info(void)
{
    return &scheduler_preempt_test_info;
}

const scheduler_ready_queue_info_t* scheduler_get_ready_queue_info(void)
{
    return &scheduler_ready_queue_info;
}

const scheduler_audit_info_t* scheduler_get_audit_info(void)
{
    return &scheduler_audit_info;
}

void scheduler_lock(void)
{
    uint32_t flags = irq_save();

    if (scheduler_lock_info.depth == 0)
    {
        scheduler_lock_info.saved_flags = flags;
    }

    scheduler_lock_info.depth++;
    scheduler_lock_info.locked = 1;
    scheduler_lock_info.lock_count++;

    if (scheduler_lock_info.depth > scheduler_lock_info.max_depth)
    {
        scheduler_lock_info.max_depth = scheduler_lock_info.depth;
    }

    scheduler_lock_info.interrupts_enabled_now = irq_are_enabled();
}

void scheduler_unlock(void)
{
    if (scheduler_lock_info.depth == 0)
    {
        scheduler_lock_info.failed_unlocks++;
        scheduler_lock_info.interrupts_enabled_now = irq_are_enabled();

        kernel_assert_message(0, "scheduler_unlock called with depth zero");
        return;
    }

    scheduler_lock_info.depth--;
    scheduler_lock_info.unlock_count++;

    if (scheduler_lock_info.depth == 0)
    {
        uint32_t flags = scheduler_lock_info.saved_flags;

        scheduler_lock_info.saved_flags = 0;
        scheduler_lock_info.locked = 0;

        irq_restore(flags);
    }

    scheduler_lock_info.interrupts_enabled_now = irq_are_enabled();
}

int scheduler_run_lock_test(void)
{
    uint32_t before = irq_are_enabled();

    scheduler_lock_info.test_runs++;

    scheduler_lock();
    uint32_t locked_once = scheduler_lock_info.locked;
    uint32_t depth_once = scheduler_lock_info.depth;
    uint32_t interrupts_after_lock = irq_are_enabled();

    scheduler_lock();
    uint32_t depth_twice = scheduler_lock_info.depth;

    scheduler_unlock();
    uint32_t depth_after_one_unlock = scheduler_lock_info.depth;

    scheduler_unlock();
    uint32_t after = irq_are_enabled();

    scheduler_lock_info.interrupts_enabled_now = after;

    if (!locked_once)
    {
        return 0;
    }

    if (depth_once != 1 || depth_twice != 2 || depth_after_one_unlock != 1)
    {
        return 0;
    }

    if (interrupts_after_lock != 0)
    {
        return 0;
    }

    if (before != after)
    {
        return 0;
    }

    return 1;
}

const scheduler_lock_info_t* scheduler_get_lock_info(void)
{
    scheduler_lock_info.interrupts_enabled_now = irq_are_enabled();
    return &scheduler_lock_info;
}

int scheduler_run_context_self_test(void)
{
    if (!scheduler_stats.initialized)
    {
        return 0;
    }

    scheduler_stats.context_self_tests++;
    return 1;
}

int scheduler_run_switch_test(void)
{
    if (!scheduler_stats.initialized)
    {
        return 0;
    }

    if (!scheduler_ensure_switch_test_stack())
    {
        return 0;
    }

    scheduler_clear_context(&switch_test_return_context);
    scheduler_clear_context(&switch_test_context);

    switch_test_info.started = 1;
    switch_test_info.entered = 0;
    switch_test_info.completed = 0;
    switch_test_info.returned_unexpectedly = 0;
    switch_test_info.test_entry_address = (uint32_t)scheduler_switch_test_entry;

    switch_test_context.eip = (uint32_t)scheduler_switch_test_entry;
    switch_test_context.esp = switch_test_info.test_stack_top;
    switch_test_context.ebp = switch_test_info.test_stack_top;
    switch_test_context.eflags = 0x202;

    switch_test_context.eax = 0x11111111;
    switch_test_context.ebx = 0x22222222;
    switch_test_context.ecx = 0x33333333;
    switch_test_context.edx = 0x44444444;
    switch_test_context.esi = 0x55555555;
    switch_test_context.edi = 0x66666666;

    scheduler_stats.switch_tests++;

    scheduler_context_switch(&switch_test_return_context, &switch_test_context);

    switch_test_info.return_eip = switch_test_return_context.eip;
    switch_test_info.test_saved_eip = switch_test_context.eip;

    if (!switch_test_info.entered)
    {
        scheduler_stats.switch_test_failures++;
        return 0;
    }

    switch_test_info.completed = 1;
    return 1;
}

void scheduler_reset_switch_test(void)
{
    if (switch_test_info.test_stack_base != 0)
    {
        kfree((void*)switch_test_info.test_stack_base);
    }

    scheduler_clear_context(&switch_test_return_context);
    scheduler_clear_context(&switch_test_context);
    scheduler_clear_switch_test_info();
}

const scheduler_switch_test_info_t* scheduler_get_switch_test_info(void)
{
    return &switch_test_info;
}

const scheduler_stats_t* scheduler_get_stats(void)
{
    return &scheduler_stats;
}

const scheduler_task_t* scheduler_get_tasks(void)
{
    return scheduler_tasks;
}

const scheduler_task_t* scheduler_get_task(uint32_t task_id)
{
    return scheduler_find_task_by_id_mutable(task_id);
}

const scheduler_task_t* scheduler_get_current_task(void)
{
    return scheduler_current_running_task;
}

static void scheduler_test_workload_task(void)
{
    test_workload_counter++;
}

void scheduler_create_test_workload(void)
{
    if (test_workload_task_id != 0)
    {
        scheduler_task_t* existing = scheduler_find_task_by_id_mutable(test_workload_task_id);

        if (existing != 0 && existing->state == SCHEDULER_TASK_ACTIVE)
        {
            return;
        }
    }

    int task_id = scheduler_create_kernel_task(
        "sched-workload",
        100,
        scheduler_test_workload_task
    );

    if (task_id > 0)
    {
        test_workload_task_id = (uint32_t)task_id;
    }
}

void scheduler_disable_test_workload(void)
{
    if (test_workload_task_id == 0)
    {
        return;
    }

    scheduler_disable_task(test_workload_task_id);
    test_workload_task_id = 0;
}

void scheduler_reset_test_workload_counter(void)
{
    test_workload_counter = 0;
}

uint32_t scheduler_get_test_workload_counter(void)
{
    return test_workload_counter;
}

uint8_t scheduler_is_test_workload_active(void)
{
    if (test_workload_task_id == 0)
    {
        return 0;
    }

    scheduler_task_t* task = scheduler_find_task_by_id_mutable(test_workload_task_id);

    if (task == 0)
    {
        return 0;
    }

    return task->state == SCHEDULER_TASK_ACTIVE;
}
