#ifndef LIAM_OS_PROCESS_H
#define LIAM_OS_PROCESS_H

#include "types.h"
#include "status.h"

#define PROCESS_MAX_PROCESSES 16
#define PROCESS_NAME_MAX 32
#define PROCESS_INVALID_PID 0
#define PROCESS_MAX_OPEN_FILES 16
#define PROCESS_MAX_ARGUMENTS 8
#define PROCESS_ARGUMENT_MAX 64
#define PROCESS_ARGUMENT_LINE_MAX 128

typedef struct
{
    uint8_t used;
    const void* node;
    uint32_t offset;
    uint32_t flags;
} process_file_t;

typedef enum
{
    PROCESS_UNUSED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_EXITED,
    PROCESS_FAILED
} process_state_t;

typedef struct
{
    uint32_t pid;
    char name[PROCESS_NAME_MAX];
    process_state_t state;

    uint32_t entry;
    uint32_t user_stack;

    int32_t exit_code;
    kernel_status_t last_status;
    uint32_t runs;

    uint32_t argc;
    char arguments[PROCESS_ARGUMENT_LINE_MAX];
    char argv[PROCESS_MAX_ARGUMENTS][PROCESS_ARGUMENT_MAX];

    process_file_t files[PROCESS_MAX_OPEN_FILES];
} process_t;

typedef struct
{
    uint8_t initialized;

    uint32_t table_capacity;
    uint32_t used_slots;
    uint32_t current_pid;
    uint32_t next_pid;

    uint32_t create_attempts;
    uint32_t successful_creates;
    uint32_t failed_creates;

    uint32_t run_attempts;
    uint32_t successful_runs;
    uint32_t failed_runs;

    uint32_t exit_count;
    uint32_t failed_processes;

    uint32_t last_created_pid;
    uint32_t last_run_pid;
    uint32_t last_exit_pid;
    int32_t last_exit_code;
    kernel_status_t last_exit_status;
    char last_exit_name[PROCESS_NAME_MAX];
    uint8_t has_last_exit;

    kernel_status_t last_status;
} process_info_t;

void process_initialize(void);

kernel_status_t process_create(
    const char* name,
    uint32_t entry,
    uint32_t user_stack,
    process_t** out_process
);

kernel_status_t process_configure_arguments(process_t* process, const char* arguments);
kernel_status_t process_run(process_t* process);
void process_mark_exit(int32_t exit_code);

uint32_t process_get_current_argc(void);
kernel_status_t process_get_current_argument(
    uint32_t index,
    char* buffer,
    uint32_t length,
    uint32_t* out_length
);
uint32_t process_get_current_pid(void);
const process_t* process_get_current(void);
process_t* process_get_current_mutable(void);
const process_t* process_get_table(void);
uint32_t process_get_count(void);
const process_t* process_find_by_pid(uint32_t pid);
process_t* process_find_by_pid_mutable(uint32_t pid);
process_t* process_find_next_ready(void);
kernel_status_t process_run_next_ready(process_t** out_process);
kernel_status_t process_run_by_pid(uint32_t pid, process_t** out_process);
kernel_status_t process_run_all_ready(uint32_t max_runs, uint32_t* out_run_count);
const process_info_t* process_get_info(void);
uint32_t process_clear_exited(void);

const char* process_state_name(process_state_t state);

#endif