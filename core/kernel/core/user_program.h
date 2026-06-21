#ifndef LIAM_OS_USER_PROGRAM_H
#define LIAM_OS_USER_PROGRAM_H

#include "types.h"
#include "status.h"

typedef struct
{
    uint8_t initialized;
    uint32_t available_programs;
    uint32_t runs;
    uint32_t successful_runs;
    uint32_t failed_runs;
    kernel_status_t last_status;
    char last_program_name[32];
} user_program_info_t;

void user_program_initialize(void);

uint32_t user_program_count(void);
const char* user_program_name_at(uint32_t index);

kernel_status_t user_program_run(const char* name);
const user_program_info_t* user_program_get_info(void);

#endif