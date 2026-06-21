#include "fd_table.h"
#include "string.h"

static fd_table_info_t fd_table_info;

void fd_table_initialize(void)
{
    fd_table_info.initialized = 1;

    fd_table_info.open_attempts = 0;
    fd_table_info.successful_opens = 0;
    fd_table_info.failed_opens = 0;

    fd_table_info.read_attempts = 0;
    fd_table_info.successful_reads = 0;
    fd_table_info.failed_reads = 0;

    fd_table_info.close_attempts = 0;
    fd_table_info.successful_closes = 0;
    fd_table_info.failed_closes = 0;

    fd_table_info.last_status = KERNEL_OK;
    fd_table_info.last_fd = FD_INVALID;
    fd_table_info.last_path[0] = '\0';
}

void fd_table_initialize_process(process_t* process)
{
    if (process == 0)
    {
        return;
    }

    for (uint32_t i = 0; i < PROCESS_MAX_OPEN_FILES; i++)
    {
        process->files[i].used = 0;
        process->files[i].node = 0;
        process->files[i].offset = 0;
        process->files[i].flags = 0;
    }
}

static kernel_status_t fd_table_allocate_slot(process_t* process, uint32_t* out_fd)
{
    if (process == 0 || out_fd == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    for (uint32_t i = FD_FIRST_PROCESS_FILE; i < PROCESS_MAX_OPEN_FILES; i++)
    {
        if (!process->files[i].used)
        {
            *out_fd = i;
            return KERNEL_OK;
        }
    }

    return KERNEL_ERROR_OUT_OF_MEMORY;
}

kernel_status_t fd_table_open_current(const char* path, uint32_t flags, uint32_t* out_fd)
{
    fd_table_info.open_attempts++;
    fd_table_info.last_fd = FD_INVALID;

    if (path != 0)
    {
        string_copy(fd_table_info.last_path, path, VFS_PATH_MAX);
    }
    else
    {
        fd_table_info.last_path[0] = '\0';
    }

    if (!fd_table_info.initialized)
    {
        fd_table_info.failed_opens++;
        fd_table_info.last_status = KERNEL_ERROR_NOT_INITIALIZED;
        return fd_table_info.last_status;
    }

    process_t* process = process_get_current_mutable();

    if (process == 0 || path == 0 || out_fd == 0)
    {
        fd_table_info.failed_opens++;
        fd_table_info.last_status = KERNEL_ERROR_INVALID_ARGUMENT;
        return fd_table_info.last_status;
    }

    const vfs_node_t* node = 0;
    kernel_status_t status = vfs_lookup(path, &node);

    if (status != KERNEL_OK)
    {
        fd_table_info.failed_opens++;
        fd_table_info.last_status = status;
        return status;
    }

    if (node->type != VFS_NODE_FILE)
    {
        fd_table_info.failed_opens++;
        fd_table_info.last_status = KERNEL_ERROR_INVALID_STATE;
        return fd_table_info.last_status;
    }

    uint32_t fd = FD_INVALID;
    status = fd_table_allocate_slot(process, &fd);

    if (status != KERNEL_OK)
    {
        fd_table_info.failed_opens++;
        fd_table_info.last_status = status;
        return status;
    }

    process->files[fd].used = 1;
    process->files[fd].node = node;
    process->files[fd].offset = 0;
    process->files[fd].flags = flags;

    *out_fd = fd;

    fd_table_info.successful_opens++;
    fd_table_info.last_fd = fd;
    fd_table_info.last_status = KERNEL_OK;

    return KERNEL_OK;
}

kernel_status_t fd_table_read_current(
    uint32_t fd,
    uint8_t* buffer,
    uint32_t length,
    uint32_t* out_bytes_read
)
{
    fd_table_info.read_attempts++;
    fd_table_info.last_fd = fd;

    if (out_bytes_read != 0)
    {
        *out_bytes_read = 0;
    }

    if (!fd_table_info.initialized)
    {
        fd_table_info.failed_reads++;
        fd_table_info.last_status = KERNEL_ERROR_NOT_INITIALIZED;
        return fd_table_info.last_status;
    }

    process_t* process = process_get_current_mutable();

    if (process == 0 || buffer == 0 || out_bytes_read == 0)
    {
        fd_table_info.failed_reads++;
        fd_table_info.last_status = KERNEL_ERROR_INVALID_ARGUMENT;
        return fd_table_info.last_status;
    }

    if (
        fd >= PROCESS_MAX_OPEN_FILES ||
        fd < FD_FIRST_PROCESS_FILE ||
        !process->files[fd].used
    )
    {
        fd_table_info.failed_reads++;
        fd_table_info.last_status = KERNEL_ERROR_INVALID_ARGUMENT;
        return fd_table_info.last_status;
    }

    process_file_t* entry = &process->files[fd];
    const vfs_node_t* node = (const vfs_node_t*)entry->node;

    if (node == 0 || node->type != VFS_NODE_FILE)
    {
        fd_table_info.failed_reads++;
        fd_table_info.last_status = KERNEL_ERROR_INVALID_STATE;
        return fd_table_info.last_status;
    }

    kernel_status_t status = vfs_read_node(
        node,
        entry->offset,
        buffer,
        length,
        out_bytes_read
    );

    if (status != KERNEL_OK)
    {
        fd_table_info.failed_reads++;
        fd_table_info.last_status = status;
        return status;
    }

    entry->offset += *out_bytes_read;

    fd_table_info.successful_reads++;
    fd_table_info.last_status = KERNEL_OK;

    return KERNEL_OK;
}

kernel_status_t fd_table_close_current(uint32_t fd)
{
    fd_table_info.close_attempts++;
    fd_table_info.last_fd = fd;

    if (!fd_table_info.initialized)
    {
        fd_table_info.failed_closes++;
        fd_table_info.last_status = KERNEL_ERROR_NOT_INITIALIZED;
        return fd_table_info.last_status;
    }

    process_t* process = process_get_current_mutable();

    if (
        process == 0 ||
        fd >= PROCESS_MAX_OPEN_FILES ||
        fd < FD_FIRST_PROCESS_FILE ||
        !process->files[fd].used
    )
    {
        fd_table_info.failed_closes++;
        fd_table_info.last_status = KERNEL_ERROR_INVALID_ARGUMENT;
        return fd_table_info.last_status;
    }

    process->files[fd].used = 0;
    process->files[fd].node = 0;
    process->files[fd].offset = 0;
    process->files[fd].flags = 0;

    fd_table_info.successful_closes++;
    fd_table_info.last_status = KERNEL_OK;

    return KERNEL_OK;
}

const fd_table_info_t* fd_table_get_info(void)
{
    return &fd_table_info;
}