#include "boot.h"
#include "log.h"

typedef struct
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} multiboot_info_t;

typedef struct
{
    uint32_t size;
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed)) multiboot_memory_map_entry_t;

static boot_info_t boot_info;

void boot_initialize(uint32_t magic, uint32_t info_address)
{
    boot_info.magic = magic;
    boot_info.info_address = info_address;
    boot_info.is_valid_multiboot = magic == MULTIBOOT_BOOTLOADER_MAGIC;

    boot_info.has_memory_info = 0;
    boot_info.memory_lower_kb = 0;
    boot_info.memory_upper_kb = 0;

    boot_info.has_memory_map = 0;
    boot_info.memory_map_length = 0;
    boot_info.memory_map_address = 0;
    boot_info.memory_map_region_count = 0;

    if (!boot_info.is_valid_multiboot || info_address == 0)
    {
        return;
    }

    const multiboot_info_t *multiboot_info = (const multiboot_info_t *)info_address;

    if ((multiboot_info->flags & MULTIBOOT_INFO_MEMORY_FLAG) != 0)
    {
        boot_info.has_memory_info = 1;
        boot_info.memory_lower_kb = multiboot_info->mem_lower;
        boot_info.memory_upper_kb = multiboot_info->mem_upper;
    }

    if ((multiboot_info->flags & MULTIBOOT_INFO_MEM_MAP_FLAG) != 0)
    {
        boot_info.has_memory_map = 1;
        boot_info.memory_map_length = multiboot_info->mmap_length;
        boot_info.memory_map_address = multiboot_info->mmap_addr;

        uint32_t current = boot_info.memory_map_address;
        uint32_t end = boot_info.memory_map_address + boot_info.memory_map_length;

        while (current < end)
        {
            const multiboot_memory_map_entry_t *entry =
                (const multiboot_memory_map_entry_t *)current;

            boot_info.memory_map_region_count++;
            current += entry->size + sizeof(entry->size);
        }
    }
}

const boot_info_t *boot_get_info(void)
{
    return &boot_info;
}

void boot_print_info(void)
{
    if (boot_info.is_valid_multiboot)
    {
        log_success("Valid Multiboot boot detected");
    }
    else
    {
        log_error("Invalid Multiboot boot detected");
    }

    log_debug_hex_u32("Multiboot magic", boot_info.magic);
    log_debug_hex_u32("Multiboot info address", boot_info.info_address);

    if (boot_info.has_memory_info)
    {
        log_success("Basic memory information detected");
        log_debug_u32("Lower memory KB", boot_info.memory_lower_kb);
        log_debug_u32("Upper memory KB", boot_info.memory_upper_kb);
    }
    else
    {
        log_warning("Basic memory information unavailable");
    }

    if (!boot_info.has_memory_map)
    {
        log_warning("Memory map unavailable");
        return;
    }

    log_success("Memory map detected");
    log_info_u32("Memory map regions", boot_info.memory_map_region_count);
    log_debug_u32("Memory map length", boot_info.memory_map_length);
    log_debug_hex_u32("Memory map address", boot_info.memory_map_address);
}