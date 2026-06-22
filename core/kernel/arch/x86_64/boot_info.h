#ifndef LIAM_OS_X86_64_BOOT_INFO_H
#define LIAM_OS_X86_64_BOOT_INFO_H

#include "types.h"

#define X86_64_BOOTLOADER_NAME_MAX 64U
#define X86_64_BOOT_COMMAND_LINE_MAX 128U
#define X86_64_MEMORY_REGION_MAX 32U
#define X86_64_MEMORY_REGION_AVAILABLE 1U

extern u32 x86_64_exception_self_test_requested;
extern u32 x86_64_irq_self_test_requested;

struct x86_64_memory_region {
    u64 base;
    u64 length;
    u32 type;
};

struct x86_64_boot_summary {
    u32 magic;
    u32 boot_info_addr;
    u32 multiboot2_valid;
    u32 total_size;
    u32 command_line_found;
    char command_line[X86_64_BOOT_COMMAND_LINE_MAX];
    u32 exception_test_requested;
    u32 irq_test_requested;
    u32 bootloader_name_found;
    char bootloader_name[X86_64_BOOTLOADER_NAME_MAX];
    u32 basic_meminfo_found;
    u32 mem_lower_kib;
    u32 mem_upper_kib;
    u32 mmap_found;
    u32 mmap_entry_count;
    u32 memory_region_count;
    u64 usable_memory_bytes;
    struct x86_64_memory_region memory_regions[X86_64_MEMORY_REGION_MAX];
};

void x86_64_boot_info_parse(u32 magic, u32 boot_info_addr, struct x86_64_boot_summary *summary);

#endif
