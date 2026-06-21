#include "exec.h"
#include "user_image.h"
#include "string.h"

#define EXEC_INIT_PATH "/sbin/init"

static exec_info_t exec_info;

void exec_initialize(void)
{
    exec_info.initialized = 1;
    exec_info.spawn_attempts = 0;
    exec_info.successful_spawns = 0;
    exec_info.failed_spawns = 0;
    exec_info.last_status = KERNEL_OK;
    exec_info.last_pid = PROCESS_INVALID_PID;
    exec_info.last_path[0] = '\0';
}

kernel_status_t exec_create_with_args(
    const char* path,
    const char* arguments,
    process_t** out_process
)
{
    if (path == 0 || out_process == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    exec_info.spawn_attempts++;
    string_copy(exec_info.last_path, path, 64);

    user_image_t image;
    kernel_status_t status = user_image_lookup(path, &image);

    if (status != KERNEL_OK)
    {
        exec_info.failed_spawns++;
        exec_info.last_status = status;
        exec_info.last_pid = PROCESS_INVALID_PID;
        return status;
    }

    process_t* process = 0;

    status = process_create(
        image.name,
        image.entry,
        image.user_stack_top,
        &process
    );

    if (status != KERNEL_OK)
    {
        exec_info.failed_spawns++;
        exec_info.last_status = status;
        exec_info.last_pid = PROCESS_INVALID_PID;
        return status;
    }

    status = process_configure_image_path(process, path);

    if (status != KERNEL_OK)
    {
        exec_info.failed_spawns++;
        exec_info.last_status = status;
        exec_info.last_pid = process->pid;
        *out_process = process;
        return status;
    }

    status = process_configure_arguments(process, arguments);

    if (status != KERNEL_OK)
    {
        exec_info.failed_spawns++;
        exec_info.last_status = status;
        exec_info.last_pid = process->pid;
        *out_process = process;
        return status;
    }

    exec_info.successful_spawns++;
    exec_info.last_status = KERNEL_OK;
    exec_info.last_pid = process->pid;

    *out_process = process;
    return KERNEL_OK;
}

kernel_status_t exec_spawn_with_args(
    const char* path,
    const char* arguments,
    process_t** out_process
)
{
    process_t* process = 0;
    kernel_status_t status = exec_create_with_args(path, arguments, &process);

    if (status != KERNEL_OK)
    {
        if (out_process != 0)
        {
            *out_process = process;
        }

        return status;
    }

    status = process_run(process);
    exec_info.last_status = status;
    exec_info.last_pid = process->pid;

    if (status != KERNEL_OK)
    {
        exec_info.failed_spawns++;
    }

    if (out_process != 0)
    {
        *out_process = process;
    }

    return status;
}

kernel_status_t exec_spawn(const char* path, process_t** out_process)
{
    return exec_spawn_with_args(path, "", out_process);
}

kernel_status_t exec_start_init(void)
{
    process_t* process = 0;
    return exec_spawn(EXEC_INIT_PATH, &process);
}

const exec_info_t* exec_get_info(void)
{
    return &exec_info;
}
