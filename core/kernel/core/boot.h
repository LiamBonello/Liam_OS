#ifndef LIAM_OS_BOOT_H
#define LIAM_OS_BOOT_H

#include "types.h"


#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MULTIBOOT_INFO_MEMORY_FLAG 0x00000001
#define MULTIBOOT_INFO_MEM_MAP_FLAG 0x00000040

#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_MEMORY_RESERVED 2

typedef struct {
    uint32_t magic;
    uint32_t info_address;
    uint8_t is_valid_multiboot;

    uint8_t has_memory_info;
    uint32_t memory_lower_kb;
    uint32_t memory_upper_kb;

    uint8_t has_memory_map;
    uint32_t memory_map_length;
    uint32_t memory_map_address;
    uint32_t memory_map_region_count;
} boot_info_t;

void boot_initialize(uint32_t magic, uint32_t info_address);
const boot_info_t* boot_get_info(void);
void boot_print_info(void);

#endif