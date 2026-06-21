#ifndef LIAM_OS_VMM_H
#define LIAM_OS_VMM_H

#include "../core/types.h"

#include "../core/status.h"


#define VMM_PAGE_PRESENT 0x001
#define VMM_PAGE_WRITABLE 0x002
#define VMM_PAGE_USER 0x004

typedef struct
{
    uint8_t initialized;
    uint32_t mapped_pages;
    uint32_t created_page_tables;
    uint32_t last_mapped_virtual_address;
    uint32_t last_mapped_physical_address;

    uint32_t failed_map_count;
    uint32_t failed_unmap_count;
    uint32_t remap_count;
    uint32_t invalid_map_count;
    uint32_t missing_unmap_count;
} vmm_stats_t;

typedef struct
{
    uint32_t audits_run;
    uint32_t issues_found;

    uint32_t present_directory_entries;
    uint32_t present_page_entries;
    uint32_t writable_page_entries;
    uint32_t user_page_entries;

    uint32_t invalid_directory_entries;
    uint32_t invalid_page_table_addresses;
    uint32_t last_mapping_missing;
} vmm_audit_info_t;

void vmm_initialize(void);

int vmm_map_page(uint32_t virtual_address, uint32_t physical_address, uint32_t flags);
int vmm_unmap_page(uint32_t virtual_address);
int vmm_is_mapped(uint32_t virtual_address);
uint32_t vmm_get_physical_address(uint32_t virtual_address);
int vmm_validate_mapping(uint32_t virtual_address, uint32_t expected_flags);
kernel_status_t vmm_validate_mapping_status(uint32_t virtual_address);
uint32_t vmm_audit(void);
const vmm_audit_info_t* vmm_get_audit_info(void);

const vmm_stats_t* vmm_get_stats(void);
void vmm_print_stats(void);

#endif
