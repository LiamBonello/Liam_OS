#ifndef LIAM_OS_KERNEL_SYMBOLS_H
#define LIAM_OS_KERNEL_SYMBOLS_H

#include "types.h"


extern uint32_t __kernel_start;
extern uint32_t __kernel_end;

uint32_t kernel_get_start_address(void);
uint32_t kernel_get_end_address(void);

#endif