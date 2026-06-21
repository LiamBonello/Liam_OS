#include "syscall.h"
#include "status.h"
#include "log.h"
#include "process.h"
#include "fd_table.h"
#include "exec.h"
#include "../fs/vfs.h"
#include "../drivers/vga.h"
#include "../drivers/timer.h"

#define SYSCALL_WRITE_MAX_BYTES 4096

static syscall_info_t syscall_info;

void syscall_initialize(void)
{
    syscall_info.total_calls = 0;
    syscall_info.last_number = 0;
    syscall_info.last_arg1 = 0;
    syscall_info.last_arg2 = 0;
    syscall_info.last_arg3 = 0;
    syscall_info.unsupported_calls = 0;
}

static uint32_t syscall_write(const char* buffer, uint32_t length)
{
    if (buffer == NULL || length == 0)
    {
        return 0;
    }

    if (length > SYSCALL_WRITE_MAX_BYTES)
    {
        length = SYSCALL_WRITE_MAX_BYTES;
    }

    for (uint32_t i = 0; i < length; i++)
    {
        vga_write_char(buffer[i]);
    }

    return length;
}

static uint32_t syscall_open(const char* path, uint32_t flags)
{
    uint32_t fd = FD_INVALID;
    kernel_status_t status = fd_table_open_current(path, flags, &fd);

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    return fd;
}

static uint32_t syscall_read(uint32_t fd, uint8_t* buffer, uint32_t length)
{
    uint32_t bytes_read = 0;
    kernel_status_t status = fd_table_read_current(fd, buffer, length, &bytes_read);

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    return bytes_read;
}

static uint32_t syscall_close(uint32_t fd)
{
    return (uint32_t)fd_table_close_current(fd);
}

static uint32_t syscall_stat(const char* path, vfs_stat_t* out_stat)
{
    return (uint32_t)vfs_stat_path(path, out_stat);
}

static uint32_t syscall_get_arg(uint32_t index, char* buffer, uint32_t length)
{
    uint32_t argument_length = 0;
    kernel_status_t status = process_get_current_argument(
        index,
        buffer,
        length,
        &argument_length
    );

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    return argument_length;
}

static uint32_t syscall_exec(const char* path, const char* arguments)
{
    process_t* process = 0;
    kernel_status_t status = exec_create_with_args(path, arguments, &process);

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    if (process == 0)
    {
        return (uint32_t)KERNEL_ERROR_INVALID_STATE;
    }

    return process->pid;
}

uint32_t syscall_dispatch(uint32_t number, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
    syscall_info.total_calls++;
    syscall_info.last_number = number;
    syscall_info.last_arg1 = arg1;
    syscall_info.last_arg2 = arg2;
    syscall_info.last_arg3 = arg3;

    switch (number)
    {
    case LIAM_SYSCALL_EXIT:
        process_mark_exit((int32_t)arg1);
        return KERNEL_OK;

    case LIAM_SYSCALL_WRITE:
        return syscall_write((const char*)arg1, arg2);

    case LIAM_SYSCALL_GET_TICKS:
        return timer_get_ticks();

    case LIAM_SYSCALL_YIELD:
        return KERNEL_OK;

    case LIAM_SYSCALL_GET_PID:
        return process_get_current_pid();

    case LIAM_SYSCALL_OPEN:
        return syscall_open((const char*)arg1, arg2);

    case LIAM_SYSCALL_READ:
        return syscall_read(arg1, (uint8_t*)arg2, arg3);

    case LIAM_SYSCALL_CLOSE:
        return syscall_close(arg1);

    case LIAM_SYSCALL_STAT:
        return syscall_stat((const char*)arg1, (vfs_stat_t*)arg2);

    case LIAM_SYSCALL_GET_ARGC:
        return process_get_current_argc();

    case LIAM_SYSCALL_GET_ARG:
        return syscall_get_arg(arg1, (char*)arg2, arg3);

    case LIAM_SYSCALL_EXEC:
        return syscall_exec((const char*)arg1, (const char*)arg2);

    default:
        syscall_info.unsupported_calls++;
        log_warning("Unsupported syscall requested");
        return (uint32_t)KERNEL_ERROR_UNSUPPORTED;
    }
}

const syscall_info_t* syscall_get_info(void)
{
    return &syscall_info;
}
