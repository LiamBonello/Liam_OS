#ifndef LIAM_OS_RING3_H
#define LIAM_OS_RING3_H

#include "../../core/types.h"
#include "../../core/status.h"

#define RING3_USER_CODE_VIRTUAL  0x10000000
#define RING3_USER_STACK_VIRTUAL 0x10001000
#define RING3_USER_PAGE_SIZE     4096

typedef struct
{
    uint32_t entry;
    uint32_t user_stack_top;
    uint32_t image_size;
} ring3_loaded_image_t;

typedef struct
{
    uint8_t initialized;
    uint8_t ready;
    uint8_t in_progress;
    uint8_t completed;

    uint32_t runs;
    uint32_t successful_runs;
    uint32_t failed_runs;
    uint32_t syscall_count;
    uint32_t last_syscall_number;

    uint32_t load_attempts;
    uint32_t successful_loads;
    uint32_t failed_loads;

    uint32_t user_code_virtual;
    uint32_t user_stack_virtual;
    uint32_t user_stack_top;
    uint32_t code_physical;
    uint32_t stack_physical;
    uint32_t kernel_syscall_stack_top;
    uint32_t previous_tss_esp0;
    uint32_t loaded_image_size;

    kernel_status_t last_status;
} ring3_info_t;

kernel_status_t ring3_initialize(void);
kernel_status_t ring3_load_user_image(const uint8_t* image, uint32_t image_size, ring3_loaded_image_t* out_loaded);
kernel_status_t ring3_run_user_image(uint32_t user_eip, uint32_t user_esp);
const ring3_info_t* ring3_get_info(void);

uint32_t ring3_syscall_entry(uint32_t syscall_number, uint32_t arg1, uint32_t arg2, uint32_t arg3);

#endif
