#include "syscall.h"
#include "status.h"
#include "log.h"
#include "process.h"
#include "fd_table.h"
#include "exec.h"
#include "../fs/vfs.h"
#include "../drivers/vga.h"
#include "../drivers/timer.h"
#include "../arch/i386/ring3.h"

#define SYSCALL_WRITE_MAX_BYTES 4096
#define SYSCALL_COPY_CHUNK_MAX 512
#define SYSCALL_EXEC_ARGUMENT_MAX PROCESS_ARGUMENT_LINE_MAX

static syscall_info_t syscall_info;
static uint8_t syscall_copy_buffer[SYSCALL_COPY_CHUNK_MAX];

static int syscall_user_range_contains(
    uint32_t address,
    uint32_t length,
    uint32_t range_start,
    uint32_t range_length
)
{
    if (length == 0)
    {
        return 1;
    }

    uint32_t range_end = range_start + range_length - 1;
    uint32_t end = address + length - 1;

    if (end < address)
    {
        return 0;
    }

    return address >= range_start && end <= range_end;
}

static int syscall_user_buffer_is_valid(const void* pointer, uint32_t length)
{
    if (pointer == 0 || length == 0)
    {
        return 0;
    }

    uint32_t address = (uint32_t)pointer;

    if (syscall_user_range_contains(address, length, RING3_USER_CODE_VIRTUAL, RING3_USER_PAGE_SIZE))
    {
        return 1;
    }

    if (syscall_user_range_contains(address, length, RING3_USER_STACK_VIRTUAL, RING3_USER_PAGE_SIZE))
    {
        return 1;
    }

    return 0;
}

static kernel_status_t syscall_copy_from_user(void* destination, const void* source, uint32_t length)
{
    if (length == 0)
    {
        return KERNEL_OK;
    }

    if (destination == 0 || !syscall_user_buffer_is_valid(source, length))
    {
        return KERNEL_ERROR_PERMISSION_DENIED;
    }

    uint8_t* kernel_destination = (uint8_t*)destination;
    const uint8_t* user_source = (const uint8_t*)source;

    for (uint32_t i = 0; i < length; i++)
    {
        kernel_destination[i] = user_source[i];
    }

    return KERNEL_OK;
}

static kernel_status_t syscall_copy_to_user(void* destination, const void* source, uint32_t length)
{
    if (length == 0)
    {
        return KERNEL_OK;
    }

    if (!syscall_user_buffer_is_valid(destination, length) || source == 0)
    {
        return KERNEL_ERROR_PERMISSION_DENIED;
    }

    uint8_t* user_destination = (uint8_t*)destination;
    const uint8_t* kernel_source = (const uint8_t*)source;

    for (uint32_t i = 0; i < length; i++)
    {
        user_destination[i] = kernel_source[i];
    }

    return KERNEL_OK;
}

static kernel_status_t syscall_copy_user_string(
    char* destination,
    const char* source,
    uint32_t max_length
)
{
    if (destination == 0 || source == 0 || max_length == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0; i < max_length; i++)
    {
        if (!syscall_user_buffer_is_valid(source + i, 1))
        {
            destination[0] = '\0';
            return KERNEL_ERROR_PERMISSION_DENIED;
        }

        destination[i] = source[i];

        if (destination[i] == '\0')
        {
            return KERNEL_OK;
        }
    }

    destination[max_length - 1] = '\0';
    return KERNEL_ERROR_INVALID_ARGUMENT;
}

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

    uint32_t total_written = 0;

    while (total_written < length)
    {
        uint32_t chunk_length = length - total_written;

        if (chunk_length > SYSCALL_COPY_CHUNK_MAX)
        {
            chunk_length = SYSCALL_COPY_CHUNK_MAX;
        }

        kernel_status_t status = syscall_copy_from_user(
            syscall_copy_buffer,
            buffer + total_written,
            chunk_length
        );

        if (status != KERNEL_OK)
        {
            return (uint32_t)status;
        }

        for (uint32_t i = 0; i < chunk_length; i++)
        {
            vga_write_char((char)syscall_copy_buffer[i]);
        }

        total_written += chunk_length;
    }

    return total_written;
}

static uint32_t syscall_open(const char* path, uint32_t flags)
{
    char kernel_path[VFS_PATH_MAX];
    kernel_status_t status = syscall_copy_user_string(kernel_path, path, VFS_PATH_MAX);

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    uint32_t fd = FD_INVALID;
    status = fd_table_open_current(kernel_path, flags, &fd);

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    return fd;
}

static uint32_t syscall_read(uint32_t fd, uint8_t* buffer, uint32_t length)
{
    if (length == 0)
    {
        return 0;
    }

    uint32_t read_length = length;

    if (read_length > SYSCALL_COPY_CHUNK_MAX)
    {
        read_length = SYSCALL_COPY_CHUNK_MAX;
    }

    if (!syscall_user_buffer_is_valid(buffer, read_length))
    {
        return (uint32_t)KERNEL_ERROR_PERMISSION_DENIED;
    }

    uint32_t bytes_read = 0;
    kernel_status_t status = fd_table_read_current(fd, syscall_copy_buffer, read_length, &bytes_read);

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    status = syscall_copy_to_user(buffer, syscall_copy_buffer, bytes_read);

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
    char kernel_path[VFS_PATH_MAX];
    kernel_status_t status = syscall_copy_user_string(kernel_path, path, VFS_PATH_MAX);

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    vfs_stat_t kernel_stat;
    status = vfs_stat_path(kernel_path, &kernel_stat);

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    return (uint32_t)syscall_copy_to_user(out_stat, &kernel_stat, sizeof(vfs_stat_t));
}

static uint32_t syscall_get_arg(uint32_t index, char* buffer, uint32_t length)
{
    if (length == 0)
    {
        return (uint32_t)KERNEL_ERROR_INVALID_ARGUMENT;
    }

    char kernel_argument[PROCESS_ARGUMENT_MAX];
    uint32_t argument_length = 0;
    kernel_status_t status = process_get_current_argument(
        index,
        kernel_argument,
        PROCESS_ARGUMENT_MAX,
        &argument_length
    );

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    if (argument_length + 1 > length)
    {
        return (uint32_t)KERNEL_ERROR_UNSUPPORTED;
    }

    status = syscall_copy_to_user(buffer, kernel_argument, argument_length + 1);

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    return argument_length;
}

static uint32_t syscall_exec(const char* path, const char* arguments)
{
    char kernel_path[VFS_PATH_MAX];
    kernel_status_t status = syscall_copy_user_string(kernel_path, path, VFS_PATH_MAX);

    if (status != KERNEL_OK)
    {
        return (uint32_t)status;
    }

    char kernel_arguments[SYSCALL_EXEC_ARGUMENT_MAX];
    const char* kernel_argument_pointer = 0;

    if (arguments != 0)
    {
        status = syscall_copy_user_string(
            kernel_arguments,
            arguments,
            SYSCALL_EXEC_ARGUMENT_MAX
        );

        if (status != KERNEL_OK)
        {
            return (uint32_t)status;
        }

        kernel_argument_pointer = kernel_arguments;
    }

    process_t* process = 0;
    status = exec_create_with_args(kernel_path, kernel_argument_pointer, &process);

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
