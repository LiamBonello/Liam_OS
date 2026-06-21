#ifndef LIAM_OS_X86_64_MEMORY_LAYOUT_H
#define LIAM_OS_X86_64_MEMORY_LAYOUT_H

#include "types.h"

#define X86_64_KERNEL_LOAD_BASE 0x00100000ULL
#define X86_64_BOOTSTRAP_IDENTITY_MAP_BYTES 0x40000000ULL
#define X86_64_BOOT_STACK_BYTES 16384ULL
#define X86_64_PAGE_SIZE 4096ULL
#define X86_64_HUGE_PAGE_SIZE 0x200000ULL

struct x86_64_memory_layout {
    u64 kernel_load_base;
    u64 kernel_start;
    u64 kernel_end;
    u64 kernel_size_bytes;
    u64 bootstrap_identity_map_bytes;
    u64 boot_stack_bytes;
    u64 page_size;
    u64 huge_page_size;
};

void x86_64_memory_layout_init(struct x86_64_memory_layout *layout);

#endif
