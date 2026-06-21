#ifndef LIAM_OS_CONTEXT_H
#define LIAM_OS_CONTEXT_H

#include "../../core/types.h"


uint32_t context_read_eip(void);
uint32_t context_read_esp(void);
uint32_t context_read_ebp(void);
uint32_t context_read_eflags(void);

#endif