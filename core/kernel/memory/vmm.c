#include "vmm.h"
#include "pmm.h"
#include "../arch/i386/paging.h"
#include "../core/log.h"
#include "../core/assert.h"
#include "../core/status.h"


#define PAGE_SIZE 4096
#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024

#define PAGE_DIRECTORY_INDEX(address) (((address) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(address) (((address) >> 12) & 0x3FF)
#define PAGE_ALIGN_DOWN(address) ((address) & 0xFFFFF000)
#define PAGE_OFFSET(address) ((address) & 0xFFF)
#define VMM_ALLOWED_FLAGS (VMM_PAGE_WRITABLE | VMM_PAGE_USER)

static vmm_stats_t vmm_stats;
static vmm_audit_info_t vmm_audit_info;

extern uint32_t paging_get_directory_address(void);
extern void paging_invalidate_page(uint32_t virtual_address);

static uint32_t* vmm_get_page_directory(void)
{
    return (uint32_t*)paging_get_directory_address();
}

static int vmm_address_is_page_aligned(uint32_t address)
{
    return PAGE_OFFSET(address) == 0;
}

static int vmm_flags_are_valid(uint32_t flags)
{
    return (flags & ~VMM_ALLOWED_FLAGS) == 0;
}

static void vmm_clear_audit_snapshot(void)
{
    vmm_audit_info.issues_found = 0;

    vmm_audit_info.present_directory_entries = 0;
    vmm_audit_info.present_page_entries = 0;
    vmm_audit_info.writable_page_entries = 0;
    vmm_audit_info.user_page_entries = 0;

    vmm_audit_info.invalid_directory_entries = 0;
    vmm_audit_info.invalid_page_table_addresses = 0;
    vmm_audit_info.last_mapping_missing = 0;
}

static void vmm_audit_record_issue(uint32_t* counter)
{
    vmm_audit_info.issues_found++;

    if (counter != 0)
    {
        (*counter)++;
    }
}

static void vmm_clear_page_table(uint32_t* page_table)
{
    for (uint32_t i = 0; i < PAGE_TABLE_ENTRIES; i++)
    {
        page_table[i] = 0;
    }
}

static uint32_t* vmm_get_or_create_page_table(uint32_t virtual_address, uint32_t flags)
{
    uint32_t* page_directory = vmm_get_page_directory();
    uint32_t directory_index = PAGE_DIRECTORY_INDEX(virtual_address);

    if ((page_directory[directory_index] & VMM_PAGE_PRESENT) != 0)
    {
        if ((flags & VMM_PAGE_USER) != 0)
        {
            page_directory[directory_index] |= VMM_PAGE_USER;
        }

        return (uint32_t*)(page_directory[directory_index] & 0xFFFFF000);
    }

    void* page_table_page = pmm_alloc_page();

    if (page_table_page == 0)
    {
        return 0;
    }

    uint32_t* page_table = (uint32_t*)page_table_page;
    vmm_clear_page_table(page_table);

    page_directory[directory_index] =
        ((uint32_t)page_table_page & 0xFFFFF000) |
        VMM_PAGE_PRESENT |
        VMM_PAGE_WRITABLE |
        (flags & VMM_PAGE_USER);

    vmm_stats.created_page_tables++;

    return page_table;
}

void vmm_initialize(void)
{
    vmm_stats.initialized = 1;
    vmm_stats.mapped_pages = 0;
    vmm_stats.created_page_tables = 0;
    vmm_stats.last_mapped_virtual_address = 0;
    vmm_stats.last_mapped_physical_address = 0;
    vmm_stats.failed_map_count = 0;
    vmm_stats.failed_unmap_count = 0;
    vmm_stats.remap_count = 0;
    vmm_stats.invalid_map_count = 0;
    vmm_stats.missing_unmap_count = 0;

    vmm_audit_info.audits_run = 0;
    vmm_clear_audit_snapshot();

    log_success("Virtual memory manager initialized");
}

int vmm_map_page(uint32_t virtual_address, uint32_t physical_address, uint32_t flags)
{
    if (!vmm_stats.initialized)
    {
        vmm_stats.failed_map_count++;
        return 0;
    }

    if (!vmm_flags_are_valid(flags))
    {
        vmm_stats.invalid_map_count++;
        vmm_stats.failed_map_count++;
        return 0;
    }

    uint32_t aligned_virtual = PAGE_ALIGN_DOWN(virtual_address);
    uint32_t aligned_physical = PAGE_ALIGN_DOWN(physical_address);

    if (!vmm_address_is_page_aligned(virtual_address) || !vmm_address_is_page_aligned(physical_address))
    {
        vmm_stats.invalid_map_count++;
    }

    uint32_t* page_table = vmm_get_or_create_page_table(aligned_virtual, flags);

    if (page_table == 0)
    {
        vmm_stats.failed_map_count++;
        return 0;
    }

    uint32_t table_index = PAGE_TABLE_INDEX(aligned_virtual);
    uint8_t was_mapped = (page_table[table_index] & VMM_PAGE_PRESENT) != 0;

    page_table[table_index] =
        aligned_physical |
        VMM_PAGE_PRESENT |
        flags;

    paging_invalidate_page(aligned_virtual);

    if (!was_mapped)
    {
        vmm_stats.mapped_pages++;
    }
    else
    {
        vmm_stats.remap_count++;
    }

    vmm_stats.last_mapped_virtual_address = aligned_virtual;
    vmm_stats.last_mapped_physical_address = aligned_physical;

    return 1;
}

int vmm_unmap_page(uint32_t virtual_address)
{
    if (!vmm_stats.initialized)
    {
        vmm_stats.failed_unmap_count++;
        return 0;
    }

    uint32_t aligned_virtual = PAGE_ALIGN_DOWN(virtual_address);
    uint32_t* page_directory = vmm_get_page_directory();
    uint32_t directory_index = PAGE_DIRECTORY_INDEX(aligned_virtual);

    if ((page_directory[directory_index] & VMM_PAGE_PRESENT) == 0)
    {
        vmm_stats.failed_unmap_count++;
        vmm_stats.missing_unmap_count++;
        return 0;
    }

    uint32_t* page_table = (uint32_t*)(page_directory[directory_index] & 0xFFFFF000);
    uint32_t table_index = PAGE_TABLE_INDEX(aligned_virtual);

    if ((page_table[table_index] & VMM_PAGE_PRESENT) == 0)
    {
        vmm_stats.failed_unmap_count++;
        vmm_stats.missing_unmap_count++;
        return 0;
    }

    page_table[table_index] = 0;
    paging_invalidate_page(aligned_virtual);

    if (vmm_stats.mapped_pages > 0)
    {
        vmm_stats.mapped_pages--;
    }

    return 1;
}

int vmm_is_mapped(uint32_t virtual_address)
{
    if (!vmm_stats.initialized)
    {
        return 0;
    }

    uint32_t aligned_virtual = PAGE_ALIGN_DOWN(virtual_address);
    uint32_t* page_directory = vmm_get_page_directory();
    uint32_t directory_index = PAGE_DIRECTORY_INDEX(aligned_virtual);

    if ((page_directory[directory_index] & VMM_PAGE_PRESENT) == 0)
    {
        return 0;
    }

    uint32_t* page_table = (uint32_t*)(page_directory[directory_index] & 0xFFFFF000);
    uint32_t table_index = PAGE_TABLE_INDEX(aligned_virtual);

    return (page_table[table_index] & VMM_PAGE_PRESENT) != 0;
}

uint32_t vmm_get_physical_address(uint32_t virtual_address)
{
    if (!vmm_stats.initialized)
    {
        return 0;
    }

    uint32_t aligned_virtual = PAGE_ALIGN_DOWN(virtual_address);
    uint32_t offset = virtual_address & 0xFFF;

    uint32_t* page_directory = vmm_get_page_directory();
    uint32_t directory_index = PAGE_DIRECTORY_INDEX(aligned_virtual);

    if ((page_directory[directory_index] & VMM_PAGE_PRESENT) == 0)
    {
        return 0;
    }

    uint32_t* page_table = (uint32_t*)(page_directory[directory_index] & 0xFFFFF000);
    uint32_t table_index = PAGE_TABLE_INDEX(aligned_virtual);

    if ((page_table[table_index] & VMM_PAGE_PRESENT) == 0)
    {
        return 0;
    }

    return (page_table[table_index] & 0xFFFFF000) + offset;
}

int vmm_validate_mapping(uint32_t virtual_address, uint32_t expected_flags)
{
    if (!vmm_stats.initialized)
    {
        return 0;
    }

    if (!vmm_flags_are_valid(expected_flags))
    {
        return 0;
    }

    uint32_t aligned_virtual = PAGE_ALIGN_DOWN(virtual_address);
    uint32_t* page_directory = vmm_get_page_directory();
    uint32_t directory_index = PAGE_DIRECTORY_INDEX(aligned_virtual);

    if ((page_directory[directory_index] & VMM_PAGE_PRESENT) == 0)
    {
        return 0;
    }

    uint32_t* page_table = (uint32_t*)(page_directory[directory_index] & 0xFFFFF000);
    uint32_t table_index = PAGE_TABLE_INDEX(aligned_virtual);
    uint32_t entry = page_table[table_index];

    if ((entry & VMM_PAGE_PRESENT) == 0)
    {
        return 0;
    }

    return (entry & expected_flags) == expected_flags;
}

kernel_status_t vmm_validate_mapping_status(uint32_t virtual_address)
{
    if (!vmm_stats.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    if ((virtual_address & 0xFFF) != 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (!vmm_is_mapped(virtual_address))
    {
        return KERNEL_ERROR_NOT_FOUND;
    }

    if (!vmm_validate_mapping(virtual_address, 0))
    {
        return KERNEL_ERROR_CORRUPTION_DETECTED;
    }

    return KERNEL_OK;
}

uint32_t vmm_audit(void)
{
    vmm_audit_info.audits_run++;
    vmm_clear_audit_snapshot();

    if (!vmm_stats.initialized)
    {
        vmm_audit_record_issue(&vmm_audit_info.invalid_directory_entries);
        return vmm_audit_info.issues_found;
    }

    uint32_t* page_directory = vmm_get_page_directory();

    if (page_directory == 0)
    {
        vmm_audit_record_issue(&vmm_audit_info.invalid_directory_entries);
        return vmm_audit_info.issues_found;
    }

    for (uint32_t directory_index = 0; directory_index < PAGE_DIRECTORY_ENTRIES; directory_index++)
    {
        uint32_t directory_entry = page_directory[directory_index];

        if ((directory_entry & VMM_PAGE_PRESENT) == 0)
        {
            continue;
        }

        vmm_audit_info.present_directory_entries++;

        if ((directory_entry & 0xFFFFF000) == 0)
        {
            vmm_audit_record_issue(&vmm_audit_info.invalid_page_table_addresses);
            continue;
        }

        uint32_t* page_table = (uint32_t*)(directory_entry & 0xFFFFF000);

        for (uint32_t table_index = 0; table_index < PAGE_TABLE_ENTRIES; table_index++)
        {
            uint32_t page_entry = page_table[table_index];

            if ((page_entry & VMM_PAGE_PRESENT) == 0)
            {
                continue;
            }

            vmm_audit_info.present_page_entries++;

            if ((page_entry & VMM_PAGE_WRITABLE) != 0)
            {
                vmm_audit_info.writable_page_entries++;
            }

            if ((page_entry & VMM_PAGE_USER) != 0)
            {
                vmm_audit_info.user_page_entries++;
            }
        }
    }

    if (
        vmm_stats.mapped_pages > 0 &&
        !vmm_is_mapped(vmm_stats.last_mapped_virtual_address)
    )
    {
        vmm_audit_record_issue(&vmm_audit_info.last_mapping_missing);
    }

    return vmm_audit_info.issues_found;
}

const vmm_audit_info_t* vmm_get_audit_info(void)
{
    return &vmm_audit_info;
}

const vmm_stats_t* vmm_get_stats(void)
{
    return &vmm_stats;
}

void vmm_print_stats(void)
{
    log_info("Virtual memory manager statistics follow");
    log_info_u32("VMM initialized", vmm_stats.initialized);
    log_info_u32("VMM mapped pages", vmm_stats.mapped_pages);
    log_info_u32("VMM created page tables", vmm_stats.created_page_tables);
    log_info_hex_u32("Last mapped virtual", vmm_stats.last_mapped_virtual_address);
    log_info_hex_u32("Last mapped physical", vmm_stats.last_mapped_physical_address);
    log_info_u32("VMM failed maps", vmm_stats.failed_map_count);
    log_info_u32("VMM failed unmaps", vmm_stats.failed_unmap_count);
    log_info_u32("VMM remaps", vmm_stats.remap_count);
    log_info_u32("VMM invalid maps", vmm_stats.invalid_map_count);
    log_info_u32("VMM missing unmaps", vmm_stats.missing_unmap_count);
}
