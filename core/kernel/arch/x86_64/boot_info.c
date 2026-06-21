#include "boot_info.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289U
#define MULTIBOOT2_TAG_TYPE_END 0U
#define MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME 2U
#define MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO 4U
#define MULTIBOOT2_TAG_TYPE_MMAP 6U

struct multiboot2_info_header {
    u32 total_size;
    u32 reserved;
};

struct multiboot2_tag {
    u32 type;
    u32 size;
};

struct multiboot2_basic_meminfo_tag {
    u32 type;
    u32 size;
    u32 mem_lower;
    u32 mem_upper;
};

struct multiboot2_mmap_tag {
    u32 type;
    u32 size;
    u32 entry_size;
    u32 entry_version;
};

struct multiboot2_mmap_entry {
    u64 base_addr;
    u64 length;
    u32 type;
    u32 reserved;
};

static u32 align8(u32 value)
{
    return (value + 7U) & ~7U;
}

static void zero_summary(struct x86_64_boot_summary *summary)
{
    u8 *bytes = (u8 *)summary;
    for (usize i = 0; i < sizeof(*summary); ++i) {
        bytes[i] = 0U;
    }
}

static void copy_bootloader_name(char *destination, const char *source, u32 source_size)
{
    u32 i = 0;
    for (; i + 1U < X86_64_BOOTLOADER_NAME_MAX && i < source_size && source[i] != '\0'; ++i) {
        destination[i] = source[i];
    }
    destination[i] = '\0';
}

static void retain_memory_region(const struct multiboot2_mmap_entry *mmap_entry, struct x86_64_boot_summary *summary)
{
    if (summary->memory_region_count >= X86_64_MEMORY_REGION_MAX) {
        return;
    }

    struct x86_64_memory_region *region = &summary->memory_regions[summary->memory_region_count++];
    region->base = mmap_entry->base_addr;
    region->length = mmap_entry->length;
    region->type = mmap_entry->type;
}

static void parse_mmap_tag(const struct multiboot2_mmap_tag *tag, struct x86_64_boot_summary *summary)
{
    if (tag->entry_size < sizeof(struct multiboot2_mmap_entry) || tag->size < sizeof(*tag)) {
        return;
    }

    const u8 *entry = (const u8 *)(tag + 1);
    const u8 *end = ((const u8 *)tag) + tag->size;

    summary->mmap_found = 1U;

    while (entry + tag->entry_size <= end) {
        const struct multiboot2_mmap_entry *mmap_entry = (const struct multiboot2_mmap_entry *)entry;

        ++summary->mmap_entry_count;
        retain_memory_region(mmap_entry, summary);

        if (mmap_entry->type == X86_64_MEMORY_REGION_AVAILABLE) {
            summary->usable_memory_bytes += mmap_entry->length;
        }

        entry += tag->entry_size;
    }
}

void x86_64_boot_info_parse(u32 magic, u32 boot_info_addr, struct x86_64_boot_summary *summary)
{
    zero_summary(summary);
    summary->magic = magic;
    summary->boot_info_addr = boot_info_addr;

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC || boot_info_addr == 0U) {
        return;
    }

    summary->multiboot2_valid = 1U;

    const struct multiboot2_info_header *header = (const struct multiboot2_info_header *)(u64)boot_info_addr;
    summary->total_size = header->total_size;

    if (header->total_size < sizeof(*header)) {
        summary->multiboot2_valid = 0U;
        return;
    }

    u32 offset = sizeof(*header);

    while (offset + sizeof(struct multiboot2_tag) <= header->total_size) {
        const struct multiboot2_tag *tag = (const struct multiboot2_tag *)(u64)(boot_info_addr + offset);

        if (tag->type == MULTIBOOT2_TAG_TYPE_END) {
            break;
        }

        if (tag->size < sizeof(*tag)) {
            summary->multiboot2_valid = 0U;
            break;
        }

        if (offset + tag->size > header->total_size) {
            summary->multiboot2_valid = 0U;
            break;
        }

        if (tag->type == MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME) {
            const char *name = (const char *)(tag + 1);
            summary->bootloader_name_found = 1U;
            copy_bootloader_name(summary->bootloader_name, name, tag->size - sizeof(*tag));
        } else if (tag->type == MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO) {
            const struct multiboot2_basic_meminfo_tag *meminfo = (const struct multiboot2_basic_meminfo_tag *)tag;
            if (meminfo->size >= sizeof(*meminfo)) {
                summary->basic_meminfo_found = 1U;
                summary->mem_lower_kib = meminfo->mem_lower;
                summary->mem_upper_kib = meminfo->mem_upper;
            }
        } else if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
            parse_mmap_tag((const struct multiboot2_mmap_tag *)tag, summary);
        }

        offset += align8(tag->size);
    }
}
