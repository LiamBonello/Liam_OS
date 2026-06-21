#ifndef LIAM_OS_FD_TABLE_H
#define LIAM_OS_FD_TABLE_H

#include "types.h"
#include "status.h"
#include "process.h"
#include "../fs/vfs.h"

#define FD_TABLE_MAX_OPEN_FILES 16
#define FD_INVALID 0xFFFFFFFFU
#define FD_FIRST_PROCESS_FILE 3

typedef struct
{
    uint8_t used;
    const vfs_node_t* node;
    uint32_t offset;
    uint32_t flags;
} fd_entry_t;

typedef struct
{
    uint8_t initialized;

    uint32_t open_attempts;
    uint32_t successful_opens;
    uint32_t failed_opens;

    uint32_t read_attempts;
    uint32_t successful_reads;
    uint32_t failed_reads;

    uint32_t close_attempts;
    uint32_t successful_closes;
    uint32_t failed_closes;

    kernel_status_t last_status;
    uint32_t last_fd;
    char last_path[VFS_PATH_MAX];
} fd_table_info_t;

void fd_table_initialize(void);
void fd_table_initialize_process(process_t* process);

kernel_status_t fd_table_open_current(const char* path, uint32_t flags, uint32_t* out_fd);
kernel_status_t fd_table_read_current(uint32_t fd, uint8_t* buffer, uint32_t length, uint32_t* out_bytes_read);
kernel_status_t fd_table_close_current(uint32_t fd);

const fd_table_info_t* fd_table_get_info(void);

#endif