#ifndef LIAM_OS_EXEC_H
#define LIAM_OS_EXEC_H

#include "types.h"
#include "status.h"
#include "process.h"

typedef struct
{
    uint8_t initialized;
    uint32_t spawn_attempts;
    uint32_t successful_spawns;
    uint32_t failed_spawns;
    kernel_status_t last_status;
    uint32_t last_pid;
    char last_path[64];
} exec_info_t;

void exec_initialize(void);
kernel_status_t exec_create_with_args(
    const char* path,
    const char* arguments,
    process_t** out_process
);

kernel_status_t exec_spawn(const char* path, process_t** out_process);
kernel_status_t exec_spawn_with_args(
    const char* path,
    const char* arguments,
    process_t** out_process
);
kernel_status_t exec_start_init(void);
const exec_info_t* exec_get_info(void);

#endif
