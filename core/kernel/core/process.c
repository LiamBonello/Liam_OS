#include "process.h"
#include "string.h"
#include "log.h"
#include "fd_table.h"
#include "../arch/i386/ring3.h"

static process_t process_table[PROCESS_MAX_PROCESSES];
static uint32_t next_pid = 1;
static process_t* current_process = 0;
static uint32_t process_count = 0;
static process_info_t process_info;

static void process_reset_slot(process_t* process);
static void process_clear_arguments(process_t* process);

void process_initialize(void)
{
    for (uint32_t i = 0; i < PROCESS_MAX_PROCESSES; i++)
    {
        process_reset_slot(&process_table[i]);
    }

    next_pid = 1;
    current_process = 0;
    process_count = 0;

    process_info.initialized = 1;
    process_info.table_capacity = PROCESS_MAX_PROCESSES;
    process_info.used_slots = 0;
    process_info.current_pid = PROCESS_INVALID_PID;
    process_info.next_pid = next_pid;

    process_info.create_attempts = 0;
    process_info.successful_creates = 0;
    process_info.failed_creates = 0;

    process_info.run_attempts = 0;
    process_info.successful_runs = 0;
    process_info.failed_runs = 0;

    process_info.exit_count = 0;
    process_info.failed_processes = 0;

    process_info.last_created_pid = PROCESS_INVALID_PID;
    process_info.last_run_pid = PROCESS_INVALID_PID;
    process_info.last_exit_pid = PROCESS_INVALID_PID;
    process_info.last_exit_code = 0;
    process_info.last_exit_status = KERNEL_OK;
    process_info.last_exit_name[0] = '\0';
    process_info.has_last_exit = 0;

    process_info.last_status = KERNEL_OK;
}

static process_t* process_find_free_slot(void)
{
    for (uint32_t i = 0; i < PROCESS_MAX_PROCESSES; i++)
    {
        if (process_table[i].state == PROCESS_UNUSED)
        {
            return &process_table[i];
        }
    }

    return 0;
}

static void process_clear_arguments(process_t* process)
{
    if (process == 0)
    {
        return;
    }

    process->argc = 0;
    string_clear(process->arguments, PROCESS_ARGUMENT_LINE_MAX);

    for (uint32_t i = 0; i < PROCESS_MAX_ARGUMENTS; i++)
    {
        string_clear(process->argv[i], PROCESS_ARGUMENT_MAX);
    }
}

static void process_reset_slot(process_t* process)
{
    if (process == 0)
    {
        return;
    }

    process->pid = PROCESS_INVALID_PID;
    process->name[0] = '\0';
    process->state = PROCESS_UNUSED;
    process->entry = 0;
    process->user_stack = 0;
    process->exit_code = 0;
    process->last_status = KERNEL_OK;
    process->runs = 0;
    process_clear_arguments(process);
    fd_table_initialize_process(process);
}

kernel_status_t process_create(
    const char* name,
    uint32_t entry,
    uint32_t user_stack,
    process_t** out_process
)
{
    process_info.create_attempts++;

    if (name == 0 || out_process == 0)
    {
        process_info.failed_creates++;
        process_info.last_status = KERNEL_ERROR_INVALID_ARGUMENT;
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (entry == 0 || user_stack == 0)
    {
        process_info.failed_creates++;
        process_info.last_status = KERNEL_ERROR_INVALID_ARGUMENT;
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    process_t* process = process_find_free_slot();

    if (process == 0)
    {
        process_info.failed_creates++;
        process_info.last_status = KERNEL_ERROR_OUT_OF_MEMORY;
        return KERNEL_ERROR_OUT_OF_MEMORY;
    }

    process->pid = next_pid++;
    string_copy(process->name, name, PROCESS_NAME_MAX);
    process->state = PROCESS_READY;
    process->entry = entry;
    process->user_stack = user_stack;
    process->exit_code = 0;
    process->last_status = KERNEL_OK;
    process->runs = 0;

    process_clear_arguments(process);
    process->argc = 1;
    string_copy(process->argv[0], process->name, PROCESS_ARGUMENT_MAX);

    fd_table_initialize_process(process);

    process_count++;
    process_info.used_slots = process_count;
    process_info.next_pid = next_pid;
    process_info.successful_creates++;
    process_info.last_created_pid = process->pid;
    process_info.last_status = KERNEL_OK;

    *out_process = process;

    return KERNEL_OK;
}

kernel_status_t process_configure_arguments(process_t* process, const char* arguments)
{
    if (process == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    process_clear_arguments(process);

    process->argc = 1;
    string_copy(process->argv[0], process->name, PROCESS_ARGUMENT_MAX);

    if (arguments == 0)
    {
        return KERNEL_OK;
    }

    const char* cursor = arguments;
    string_trim_left(&cursor);

    if (cursor[0] == '\0')
    {
        return KERNEL_OK;
    }

    string_copy(process->arguments, cursor, PROCESS_ARGUMENT_LINE_MAX);

    cursor = process->arguments;
    string_trim_left(&cursor);

    while (cursor[0] != '\0' && process->argc < PROCESS_MAX_ARGUMENTS)
    {
        uint32_t copied = string_copy_until_space(
            process->argv[process->argc],
            cursor,
            PROCESS_ARGUMENT_MAX
        );

        if (copied == 0)
        {
            break;
        }

        process->argc++;

        cursor += copied;
        string_trim_left(&cursor);
    }

    return KERNEL_OK;
}

uint32_t process_get_current_argc(void)
{
    if (current_process == 0)
    {
        return 0;
    }

    return current_process->argc;
}

kernel_status_t process_get_current_argument(
    uint32_t index,
    char* buffer,
    uint32_t length,
    uint32_t* out_length
)
{
    if (current_process == 0 || buffer == 0 || out_length == 0 || length == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (index >= current_process->argc)
    {
        return KERNEL_ERROR_NOT_FOUND;
    }

    uint32_t argument_length = string_length(current_process->argv[index]);

    if (argument_length + 1 > length)
    {
        return KERNEL_ERROR_UNSUPPORTED;
    }

    string_copy(buffer, current_process->argv[index], length);
    *out_length = argument_length;

    return KERNEL_OK;
}

kernel_status_t process_run(process_t* process)
{
    process_info.run_attempts++;

    if (process == 0)
    {
        process_info.failed_runs++;
        process_info.last_status = KERNEL_ERROR_INVALID_ARGUMENT;
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (process->state != PROCESS_READY && process->state != PROCESS_EXITED)
    {
        process_info.failed_runs++;
        process_info.last_run_pid = process->pid;
        process_info.last_status = KERNEL_ERROR_INVALID_STATE;
        return KERNEL_ERROR_INVALID_STATE;
    }

    process->state = PROCESS_RUNNING;
    process->runs++;
    process->exit_code = 0;
    process->last_status = KERNEL_OK;

    current_process = process;
    process_info.current_pid = process->pid;
    process_info.last_run_pid = process->pid;

    kernel_status_t status = ring3_run_user_image(
        process->entry,
        process->user_stack
    );

    if (process->state == PROCESS_RUNNING)
    {
        if (status == KERNEL_OK)
        {
            process->state = PROCESS_EXITED;
            process->exit_code = 0;
            process_info.exit_count++;
            process_info.last_exit_pid = process->pid;
            process_info.last_exit_code = 0;
            process_info.last_exit_status = KERNEL_OK;
            string_copy(process_info.last_exit_name, process->name, PROCESS_NAME_MAX);
            process_info.has_last_exit = 1;
        }
        else
        {
            process->state = PROCESS_FAILED;
            process->exit_code = -1;
            process_info.failed_processes++;
            process_info.last_exit_pid = process->pid;
            process_info.last_exit_code = -1;
            process_info.last_exit_status = status;
            string_copy(process_info.last_exit_name, process->name, PROCESS_NAME_MAX);
            process_info.has_last_exit = 1;
        }
    }

    process->last_status = status;
    current_process = 0;
    process_info.current_pid = PROCESS_INVALID_PID;
    process_info.last_status = status;

    if (status == KERNEL_OK)
    {
        process_info.successful_runs++;
    }
    else
    {
        process_info.failed_runs++;
    }

    return status;
}

void process_mark_exit(int32_t exit_code)
{
    if (current_process == 0)
    {
        return;
    }

    current_process->exit_code = exit_code;
    current_process->state = PROCESS_EXITED;

    process_info.exit_count++;
    process_info.last_exit_pid = current_process->pid;
    process_info.last_exit_code = exit_code;
    process_info.last_exit_status = KERNEL_OK;
    string_copy(process_info.last_exit_name, current_process->name, PROCESS_NAME_MAX);
    process_info.has_last_exit = 1;
}

uint32_t process_get_current_pid(void)
{
    if (current_process == 0)
    {
        return PROCESS_INVALID_PID;
    }

    return current_process->pid;
}

const process_t* process_get_current(void)
{
    return current_process;
}

process_t* process_get_current_mutable(void)
{
    return current_process;
}

const process_t* process_get_table(void)
{
    return process_table;
}

uint32_t process_get_count(void)
{
    return process_count;
}

const process_t* process_find_by_pid(uint32_t pid)
{
    return process_find_by_pid_mutable(pid);
}

process_t* process_find_by_pid_mutable(uint32_t pid)
{
    if (pid == PROCESS_INVALID_PID)
    {
        return 0;
    }

    for (uint32_t i = 0; i < PROCESS_MAX_PROCESSES; i++)
    {
        if (process_table[i].state == PROCESS_UNUSED)
        {
            continue;
        }

        if (process_table[i].pid == pid)
        {
            return &process_table[i];
        }
    }

    return 0;
}

process_t* process_find_next_ready(void)
{
    for (uint32_t i = 0; i < PROCESS_MAX_PROCESSES; i++)
    {
        if (process_table[i].state == PROCESS_READY)
        {
            return &process_table[i];
        }
    }

    return 0;
}

kernel_status_t process_run_next_ready(process_t** out_process)
{
    if (out_process == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    process_t* process = process_find_next_ready();

    if (process == 0)
    {
        *out_process = 0;
        return KERNEL_ERROR_NOT_FOUND;
    }

    *out_process = process;
    return process_run(process);
}

kernel_status_t process_run_by_pid(uint32_t pid, process_t** out_process)
{
    if (out_process == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    process_t* process = process_find_by_pid_mutable(pid);

    if (process == 0)
    {
        *out_process = 0;
        return KERNEL_ERROR_NOT_FOUND;
    }

    if (process->state != PROCESS_READY)
    {
        *out_process = process;
        return KERNEL_ERROR_INVALID_STATE;
    }

    *out_process = process;
    return process_run(process);
}

kernel_status_t process_run_all_ready(uint32_t max_runs, uint32_t* out_run_count)
{
    if (out_run_count == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    *out_run_count = 0;

    if (max_runs == 0)
    {
        max_runs = PROCESS_MAX_PROCESSES;
    }

    for (uint32_t i = 0; i < max_runs; i++)
    {
        process_t* process = process_find_next_ready();

        if (process == 0)
        {
            return (*out_run_count == 0)
                ? KERNEL_ERROR_NOT_FOUND
                : KERNEL_OK;
        }

        kernel_status_t status = process_run(process);

        if (status != KERNEL_OK)
        {
            return status;
        }

        (*out_run_count)++;
    }

    if (process_find_next_ready() != 0)
    {
        return KERNEL_ERROR_BUSY;
    }

    return KERNEL_OK;
}

uint32_t process_clear_exited(void)
{
    uint32_t cleared = 0;

    for (uint32_t i = 0; i < PROCESS_MAX_PROCESSES; i++)
    {
        if (process_table[i].state != PROCESS_EXITED &&
            process_table[i].state != PROCESS_FAILED)
        {
            continue;
        }

        process_reset_slot(&process_table[i]);
        cleared++;
    }

    if (cleared > process_count)
    {
        process_count = 0;
    }
    else
    {
        process_count -= cleared;
    }

    process_info.used_slots = process_count;
    process_info.last_status = KERNEL_OK;

    return cleared;
}

const process_info_t* process_get_info(void)
{
    process_info.used_slots = process_count;
    process_info.current_pid = process_get_current_pid();
    process_info.next_pid = next_pid;

    return &process_info;
}

const char* process_state_name(process_state_t state)
{
    switch (state)
    {
    case PROCESS_UNUSED:
        return "unused";
    case PROCESS_READY:
        return "ready";
    case PROCESS_RUNNING:
        return "running";
    case PROCESS_EXITED:
        return "exited";
    case PROCESS_FAILED:
        return "failed";
    default:
        return "unknown";
    }
}