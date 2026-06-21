#include "paging.h"
#include "../../core/log.h"

#define PAGE_SIZE 4096
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_DIRECTORY_ENTRIES 1024

#define IDENTITY_PAGE_TABLE_COUNT 32

#define PAGE_PRESENT 0x001
#define PAGE_WRITABLE 0x002

static uint32_t page_directory[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t identity_page_tables[IDENTITY_PAGE_TABLE_COUNT][PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

static paging_info_t paging_info;

extern void paging_load_directory(uint32_t page_directory_address);
extern void paging_enable(void);
extern uint32_t paging_read_cr0(void);
extern uint32_t paging_read_cr3(void);
extern uint32_t paging_read_cr2(void);

static void paging_clear_structures(void)
{
    for (uint32_t i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
    {
        page_directory[i] = 0;
    }

    for (uint32_t table = 0; table < IDENTITY_PAGE_TABLE_COUNT; table++)
    {
        for (uint32_t entry = 0; entry < PAGE_TABLE_ENTRIES; entry++)
        {
            identity_page_tables[table][entry] = 0;
        }
    }
}

static void paging_build_identity_map(void)
{
    for (uint32_t table = 0; table < IDENTITY_PAGE_TABLE_COUNT; table++)
    {
        for (uint32_t entry = 0; entry < PAGE_TABLE_ENTRIES; entry++)
        {
            uint32_t physical_address =
                (table * PAGE_TABLE_ENTRIES * PAGE_SIZE) +
                (entry * PAGE_SIZE);

            identity_page_tables[table][entry] =
                physical_address | PAGE_PRESENT | PAGE_WRITABLE;
        }

        page_directory[table] =
            ((uint32_t)&identity_page_tables[table]) | PAGE_PRESENT | PAGE_WRITABLE;
    }
}

void paging_initialize(void)
{
    paging_clear_structures();
    paging_build_identity_map();

    paging_info.enabled = 0;
    paging_info.mapped_start = 0x00000000;
    paging_info.mapped_bytes = IDENTITY_PAGE_TABLE_COUNT * PAGE_TABLE_ENTRIES * PAGE_SIZE;
    paging_info.mapped_end = paging_info.mapped_start + paging_info.mapped_bytes - 1;
    paging_info.page_directory_address = (uint32_t)&page_directory;
    paging_info.page_table_count = IDENTITY_PAGE_TABLE_COUNT;

    paging_load_directory((uint32_t)&page_directory);
    paging_enable();

    paging_info.enabled = 1;

    log_success("Paging enabled");
    log_info_hex_u32("Page directory", paging_info.page_directory_address);
    log_info_hex_u32("Identity map end", paging_info.mapped_end);
}

const paging_info_t* paging_get_info(void)
{
    return &paging_info;
}

uint32_t paging_get_fault_address(void)
{
    return paging_read_cr2();
}

uint32_t paging_get_directory_address(void)
{
    return paging_info.page_directory_address;
}

void paging_print_info(void)
{
    log_info("Paging information follows");
    log_info_u32("Paging enabled", paging_info.enabled);
    log_info_u32("Page tables", paging_info.page_table_count);
    log_info_hex_u32("Mapped start", paging_info.mapped_start);
    log_info_hex_u32("Mapped end", paging_info.mapped_end);
    log_info_hex_u32("CR0", paging_read_cr0());
    log_info_hex_u32("CR3", paging_read_cr3());
}