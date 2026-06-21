#include "pmm.h"
#include "../core/kernel_symbols.h"
#include "../core/log.h"

typedef struct
{
    uint32_t size;
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed)) multiboot_memory_map_entry_t;

#define MULTIBOOT_MEMORY_AVAILABLE 1
#define ONE_MIB 0x100000

static pmm_stats_t pmm_stats;
static uint32_t free_stack[PMM_MAX_FREE_PAGES];

static uint32_t align_up(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

void pmm_initialize(const boot_info_t *boot_info)
{
    pmm_stats.total_regions = 0;
    pmm_stats.usable_regions = 0;
    pmm_stats.reserved_regions = 0;
    pmm_stats.usable_memory_bytes = 0;
    pmm_stats.allocation_start = 0;
    pmm_stats.allocation_end = 0;
    pmm_stats.next_free_page = 0;
    pmm_stats.allocated_pages = 0;
    pmm_stats.free_stack_count = 0;

    uint32_t kernel_end = align_up(kernel_get_end_address(), PMM_PAGE_SIZE);

    if (boot_info == 0 || !boot_info->has_memory_map)
    {
        return;
    }

    uint32_t current = boot_info->memory_map_address;
    uint32_t end = boot_info->memory_map_address + boot_info->memory_map_length;

    while (current < end)
    {
        const multiboot_memory_map_entry_t *entry =
            (const multiboot_memory_map_entry_t *)current;

        pmm_stats.total_regions++;

        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            pmm_stats.usable_regions++;
            pmm_stats.usable_memory_bytes += entry->length;

            uint32_t base = (uint32_t)entry->base_addr;
            uint32_t length = (uint32_t)entry->length;
            uint32_t region_end = base + length;

            if (base < ONE_MIB && region_end > ONE_MIB)
            {
                base = ONE_MIB;
            }

            if (base < kernel_end && region_end > kernel_end)
            {
                base = kernel_end;
            }

            base = align_up(base, PMM_PAGE_SIZE);

            if (
                pmm_stats.allocation_start == 0 &&
                base >= ONE_MIB &&
                region_end > base)
            {
                pmm_stats.allocation_start = base;
                pmm_stats.allocation_end = region_end;
                pmm_stats.next_free_page = base;
            }
        }
        else
        {
            pmm_stats.reserved_regions++;
        }

        current += entry->size + sizeof(entry->size);
    }
}

const pmm_stats_t *pmm_get_stats(void)
{
    return &pmm_stats;
}

void pmm_print_stats(void)
{
    log_success("Physical memory manager initialized");
    log_info_u32("Usable memory KB", (uint32_t)(pmm_stats.usable_memory_bytes / 1024));
    log_info_hex_u32("PMM allocation start", pmm_stats.allocation_start);
    log_info_hex_u32("PMM allocation end", pmm_stats.allocation_end);

    log_debug_u32("PMM total regions", pmm_stats.total_regions);
    log_debug_u32("PMM usable regions", pmm_stats.usable_regions);
    log_debug_u32("PMM reserved regions", pmm_stats.reserved_regions);
    log_debug_hex_u32("Kernel start", kernel_get_start_address());
    log_debug_hex_u32("Kernel end", kernel_get_end_address());
    log_debug_hex_u32("PMM next free page", pmm_stats.next_free_page);
    log_debug_u32("PMM allocated pages", pmm_stats.allocated_pages);
    log_debug_u32("PMM free stack pages", pmm_stats.free_stack_count);
}

void *pmm_alloc_page(void)
{
    if (pmm_stats.free_stack_count > 0)
    {
        pmm_stats.free_stack_count--;

        void *page = (void *)free_stack[pmm_stats.free_stack_count];
        pmm_stats.allocated_pages++;

        return page;
    }

    if (pmm_stats.next_free_page == 0)
    {
        return 0;
    }

    if (pmm_stats.next_free_page + PMM_PAGE_SIZE > pmm_stats.allocation_end)
    {
        return 0;
    }

    void *page = (void *)pmm_stats.next_free_page;

    pmm_stats.next_free_page += PMM_PAGE_SIZE;
    pmm_stats.allocated_pages++;

    return page;
}

void pmm_free_page(void *page)
{
    if (page == 0)
    {
        return;
    }

    uint32_t page_address = (uint32_t)page;

    if ((page_address % PMM_PAGE_SIZE) != 0)
    {
        log_warning("PMM ignored unaligned free page request");
        return;
    }

    if (page_address < pmm_stats.allocation_start || page_address >= pmm_stats.allocation_end)
    {
        log_warning("PMM ignored out-of-range free page request");
        return;
    }

    if (pmm_stats.free_stack_count >= PMM_MAX_FREE_PAGES)
    {
        log_warning("PMM free stack is full");
        return;
    }

    free_stack[pmm_stats.free_stack_count] = page_address;
    pmm_stats.free_stack_count++;

    if (pmm_stats.allocated_pages > 0)
    {
        pmm_stats.allocated_pages--;
    }
}

void pmm_run_self_test(void)
{
    log_info("Running PMM allocation self-test");

    void *page1 = pmm_alloc_page();
    void *page2 = pmm_alloc_page();
    void *page3 = pmm_alloc_page();

    log_info_hex_u32("Allocated page 1", (uint32_t)page1);
    log_info_hex_u32("Allocated page 2", (uint32_t)page2);
    log_info_hex_u32("Allocated page 3", (uint32_t)page3);

    log_info_u32("PMM allocated pages after alloc", pmm_stats.allocated_pages);
    log_info_hex_u32("PMM next free page after alloc", pmm_stats.next_free_page);

    pmm_free_page(page2);

    log_info_hex_u32("Freed page", (uint32_t)page2);
    log_info_u32("PMM allocated pages after free", pmm_stats.allocated_pages);
    log_info_u32("PMM free stack pages after free", pmm_stats.free_stack_count);

    void *reused_page = pmm_alloc_page();

    log_info_hex_u32("Reused page", (uint32_t)reused_page);
    log_info_u32("PMM allocated pages after reuse", pmm_stats.allocated_pages);
    log_info_u32("PMM free stack pages after reuse", pmm_stats.free_stack_count);
}