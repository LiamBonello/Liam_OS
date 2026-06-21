#ifndef LIAM_OS_X86_64_IDT_H
#define LIAM_OS_X86_64_IDT_H

#include "types.h"

void x86_64_idt_init(void);
void x86_64_exception_handler(u64 vector, u64 error_code);

#endif
