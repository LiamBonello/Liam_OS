#ifndef LIAM_OS_TSS_H
#define LIAM_OS_TSS_H

#include "../../core/types.h"


typedef struct
{
    uint8_t initialized;
    uint8_t loaded;
    uint16_t selector;
    uint16_t ss0;
    uint32_t esp0;
    uint32_t base;
    uint32_t limit;
    uint32_t set_kernel_stack_count;
} tss_info_t;

void tss_initialize(void);
void tss_load(uint16_t selector);
void tss_set_kernel_stack(uint32_t stack_top);
uint32_t tss_get_base(void);
uint32_t tss_get_limit(void);
const tss_info_t* tss_get_info(void);

#endif
