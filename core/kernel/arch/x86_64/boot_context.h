#ifndef LIAM_OS_X86_64_BOOT_CONTEXT_H
#define LIAM_OS_X86_64_BOOT_CONTEXT_H

#include "boot_info.h"
#include "memory_layout.h"
#include "types.h"

struct x86_64_boot_context {
    struct x86_64_boot_summary boot_info;
    struct x86_64_memory_layout memory_layout;
};

void x86_64_boot_context_init(u32 magic, u32 boot_info_addr, struct x86_64_boot_context *context);

#endif
