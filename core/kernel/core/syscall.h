#ifndef LIAM_OS_SYSCALL_H
#define LIAM_OS_SYSCALL_H

#include "types.h"

typedef enum
{
    LIAM_SYSCALL_EXIT = 1,
    LIAM_SYSCALL_WRITE = 2,
    LIAM_SYSCALL_GET_TICKS = 3,
    LIAM_SYSCALL_YIELD = 4,
    LIAM_SYSCALL_GET_PID = 5,
    LIAM_SYSCALL_OPEN = 6,
    LIAM_SYSCALL_READ = 7,
    LIAM_SYSCALL_CLOSE = 8,
    LIAM_SYSCALL_STAT = 9,
    LIAM_SYSCALL_GET_ARGC = 10,
    LIAM_SYSCALL_GET_ARG = 11,
    LIAM_SYSCALL_EXEC = 12
} liam_syscall_number_t;

typedef struct
{
    uint32_t total_calls;
    uint32_t last_number;
    uint32_t last_arg1;
    uint32_t last_arg2;
    uint32_t last_arg3;
    uint32_t unsupported_calls;
} syscall_info_t;

void syscall_initialize(void);
uint32_t syscall_dispatch(uint32_t number, uint32_t arg1, uint32_t arg2, uint32_t arg3);
const syscall_info_t* syscall_get_info(void);

#endif
