#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "memory_layout.h"
#include "../core/log.h"
#include "../core/status.h"

#define HEAP_INITIAL_PAGES 1
#define HEAP_MAX_GROW_PAGES 16
#define HEAP_ALIGNMENT 8
#define HEAP_MIN_SPLIT_SIZE 16
#define HEAP_BLOCK_MAGIC 0x48454150
#define HEAP_MAX_AUDIT_BLOCKS 4096

typedef struct heap_block
{
    uint32_t magic;
    uint32_t size;
    uint8_t used;
    struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_head = 0;
static heap_stats_t heap_stats;
static heap_audit_info_t heap_audit_info;

static uint32_t align_up(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

static uint32_t heap_header_size(void)
{
    return align_up((uint32_t)sizeof(heap_block_t), HEAP_ALIGNMENT);
}

static int heap_address_in_range(uint32_t address)
{
    return address >= heap_stats.virtual_start && address <= heap_stats.virtual_end;
}

static int heap_payload_address_in_range(uint32_t address)
{
    return address >= heap_stats.virtual_start + heap_header_size() && address <= heap_stats.virtual_end;
}

static int heap_address_is_aligned(uint32_t address)
{
    return (address & (HEAP_ALIGNMENT - 1)) == 0;
}

static void heap_initialize_block(heap_block_t* block, uint32_t size)
{
    block->magic = HEAP_BLOCK_MAGIC;
    block->size = size;
    block->used = 0;
    block->next = 0;
}

static void heap_clear_audit_snapshot(void)
{
    heap_audit_info.issues_found = 0;

    heap_audit_info.blocks_seen = 0;
    heap_audit_info.used_blocks = 0;
    heap_audit_info.free_blocks = 0;

    heap_audit_info.invalid_magic_blocks = 0;
    heap_audit_info.out_of_range_blocks = 0;
    heap_audit_info.misaligned_blocks = 0;
    heap_audit_info.invalid_size_blocks = 0;
    heap_audit_info.invalid_next_links = 0;
    heap_audit_info.unmapped_heap_pages = 0;

    heap_audit_info.calculated_used_bytes = 0;
    heap_audit_info.calculated_free_bytes = 0;
    heap_audit_info.calculated_total_payload_bytes = 0;
}

static void heap_audit_record_issue(uint32_t* counter)
{
    heap_audit_info.issues_found++;

    if (counter != 0)
    {
        (*counter)++;
    }
}

static int heap_block_header_is_sane(heap_block_t* block)
{
    if (block == 0)
    {
        return 0;
    }

    uint32_t block_address = (uint32_t)block;
    uint32_t header_size = heap_header_size();

    if (!heap_address_in_range(block_address))
    {
        return 0;
    }

    if (!heap_address_is_aligned(block_address))
    {
        return 0;
    }

    if (block->magic != HEAP_BLOCK_MAGIC)
    {
        return 0;
    }

    if (block->size == 0)
    {
        return 0;
    }

    if (block_address + header_size + block->size - 1 > heap_stats.virtual_end)
    {
        return 0;
    }

    return 1;
}

static void heap_recalculate_stats(void)
{
    uint32_t used_bytes = 0;
    uint32_t free_bytes = 0;
    uint32_t allocation_count = 0;
    uint32_t free_block_count = 0;
    uint32_t visited = 0;

    heap_block_t* block = heap_head;

    while (block != 0 && visited < HEAP_MAX_AUDIT_BLOCKS)
    {
        if (!heap_block_header_is_sane(block))
        {
            break;
        }

        if (block->used)
        {
            used_bytes += block->size;
            allocation_count++;
        }
        else
        {
            free_bytes += block->size;
            free_block_count++;
        }

        block = block->next;
        visited++;
    }

    heap_stats.used_bytes = used_bytes;
    heap_stats.free_bytes = free_bytes;
    heap_stats.allocation_count = allocation_count;
    heap_stats.free_block_count = free_block_count;
}

static heap_block_t* heap_find_last_block(void)
{
    heap_block_t* block = heap_head;

    if (block == 0)
    {
        return 0;
    }

    while (block->next != 0)
    {
        block = block->next;
    }

    return block;
}

static heap_block_t* heap_find_block_for_pointer(void* pointer)
{
    if (pointer == 0 || !heap_stats.initialized)
    {
        return 0;
    }

    uint32_t pointer_address = (uint32_t)pointer;

    if (!heap_payload_address_in_range(pointer_address))
    {
        return 0;
    }

    heap_block_t* block = heap_head;
    uint32_t visited = 0;

    while (block != 0 && visited < HEAP_MAX_AUDIT_BLOCKS)
    {
        if (!heap_block_header_is_sane(block))
        {
            return 0;
        }

        uint32_t payload_address = (uint32_t)block + heap_header_size();

        if (payload_address == pointer_address)
        {
            return block;
        }

        block = block->next;
        visited++;
    }

    return 0;
}

static void heap_split_block(heap_block_t* block, uint32_t requested_size)
{
    uint32_t header_size = heap_header_size();

    if (!heap_block_header_is_sane(block))
    {
        return;
    }

    if (block->size <= requested_size + header_size + HEAP_MIN_SPLIT_SIZE)
    {
        return;
    }

    uint32_t remaining_size = block->size - requested_size - header_size;
    uint32_t new_block_address = (uint32_t)block + header_size + requested_size;

    heap_block_t* new_block = (heap_block_t*)new_block_address;
    heap_initialize_block(new_block, remaining_size);
    new_block->next = block->next;

    block->size = requested_size;
    block->next = new_block;
}

static void heap_merge_free_blocks(void)
{
    uint32_t header_size = heap_header_size();
    heap_block_t* block = heap_head;
    uint32_t visited = 0;

    while (block != 0 && block->next != 0 && visited < HEAP_MAX_AUDIT_BLOCKS)
    {
        if (!heap_block_header_is_sane(block) || !heap_block_header_is_sane(block->next))
        {
            return;
        }

        uint32_t block_end = (uint32_t)block + header_size + block->size;

        if (!block->used && !block->next->used && block_end == (uint32_t)block->next)
        {
            block->size += header_size + block->next->size;
            block->next = block->next->next;
            continue;
        }

        block = block->next;
        visited++;
    }
}

static int heap_virtual_range_has_space(uint32_t pages)
{
    uint32_t bytes = pages * PMM_PAGE_SIZE;

    if (heap_stats.next_virtual_page < heap_stats.virtual_start)
    {
        return 0;
    }

    if (heap_stats.next_virtual_page + bytes - 1 > heap_stats.virtual_end)
    {
        return 0;
    }

    return 1;
}

static void heap_rollback_growth(
    const uint32_t* virtual_pages,
    void* const* physical_pages,
    uint32_t mapped_pages,
    uint32_t original_next_virtual_page,
    uint32_t original_total_pages,
    uint32_t original_total_bytes
)
{
    for (uint32_t i = 0; i < mapped_pages; i++)
    {
        vmm_unmap_page(virtual_pages[i]);
        pmm_free_page(physical_pages[i]);
    }

    heap_stats.next_virtual_page = original_next_virtual_page;
    heap_stats.total_pages = original_total_pages;
    heap_stats.total_bytes = original_total_bytes;
}

static int heap_grow(uint32_t minimum_size)
{
    uint32_t header_size = heap_header_size();
    uint32_t required_bytes = minimum_size + header_size;
    uint32_t required_pages = align_up(required_bytes, PMM_PAGE_SIZE) / PMM_PAGE_SIZE;

    if (required_pages == 0)
    {
        required_pages = 1;
    }

    if (required_pages > HEAP_MAX_GROW_PAGES)
    {
        heap_stats.failed_grow_count++;
        return 0;
    }

    if (!heap_virtual_range_has_space(required_pages))
    {
        heap_stats.failed_grow_count++;
        return 0;
    }

    uint32_t mapped_virtual_pages[HEAP_MAX_GROW_PAGES];
    void* allocated_physical_pages[HEAP_MAX_GROW_PAGES];
    uint32_t mapped_pages = 0;

    uint32_t original_next_virtual_page = heap_stats.next_virtual_page;
    uint32_t original_total_pages = heap_stats.total_pages;
    uint32_t original_total_bytes = heap_stats.total_bytes;

    heap_block_t* first_new_block = 0;
    heap_block_t* previous_new_block = 0;

    for (uint32_t i = 0; i < required_pages; i++)
    {
        uint32_t virtual_address = heap_stats.next_virtual_page;
        void* physical_page = pmm_alloc_page();

        if (physical_page == 0)
        {
            heap_rollback_growth(
                mapped_virtual_pages,
                allocated_physical_pages,
                mapped_pages,
                original_next_virtual_page,
                original_total_pages,
                original_total_bytes
            );
            heap_stats.failed_grow_count++;
            return 0;
        }

        if (!vmm_map_page(
            virtual_address,
            (uint32_t)physical_page,
            VMM_PAGE_WRITABLE
        ))
        {
            pmm_free_page(physical_page);
            heap_rollback_growth(
                mapped_virtual_pages,
                allocated_physical_pages,
                mapped_pages,
                original_next_virtual_page,
                original_total_pages,
                original_total_bytes
            );
            heap_stats.failed_grow_count++;
            return 0;
        }

        mapped_virtual_pages[mapped_pages] = virtual_address;
        allocated_physical_pages[mapped_pages] = physical_page;
        mapped_pages++;

        heap_block_t* new_block = (heap_block_t*)virtual_address;
        heap_initialize_block(new_block, PMM_PAGE_SIZE - header_size);

        if (first_new_block == 0)
        {
            first_new_block = new_block;
        }

        if (previous_new_block != 0)
        {
            previous_new_block->next = new_block;
        }

        previous_new_block = new_block;

        heap_stats.next_virtual_page += PMM_PAGE_SIZE;
        heap_stats.total_pages++;
        heap_stats.total_bytes += PMM_PAGE_SIZE;
    }

    if (heap_head == 0)
    {
        heap_head = first_new_block;
    }
    else
    {
        heap_block_t* last = heap_find_last_block();

        if (last == 0)
        {
            heap_rollback_growth(
                mapped_virtual_pages,
                allocated_physical_pages,
                mapped_pages,
                original_next_virtual_page,
                original_total_pages,
                original_total_bytes
            );
            heap_stats.failed_grow_count++;
            return 0;
        }

        last->next = first_new_block;
    }

    heap_stats.grow_count++;

    heap_merge_free_blocks();
    heap_recalculate_stats();

    return 1;
}

void heap_initialize(void)
{
    heap_head = 0;

    heap_stats.initialized = 0;

    heap_stats.virtual_start = MEMORY_LAYOUT_KERNEL_HEAP_START;
    heap_stats.virtual_end = MEMORY_LAYOUT_KERNEL_HEAP_END;
    heap_stats.next_virtual_page = MEMORY_LAYOUT_KERNEL_HEAP_START;

    heap_stats.total_pages = 0;
    heap_stats.total_bytes = 0;
    heap_stats.used_bytes = 0;
    heap_stats.free_bytes = 0;
    heap_stats.allocation_count = 0;
    heap_stats.free_block_count = 0;
    heap_stats.grow_count = 0;
    heap_stats.failed_grow_count = 0;
    heap_stats.failed_allocation_count = 0;
    heap_stats.successful_free_count = 0;
    heap_stats.invalid_free_count = 0;
    heap_stats.double_free_count = 0;

    heap_audit_info.audits_run = 0;
    heap_clear_audit_snapshot();

    if (!heap_grow(HEAP_INITIAL_PAGES * PMM_PAGE_SIZE - heap_header_size()))
    {
        log_error("Kernel heap initialization failed");
        return;
    }

    heap_stats.initialized = 1;
    heap_recalculate_stats();

    log_success("Kernel heap initialized");
    log_info_hex_u32("Heap virtual start", heap_stats.virtual_start);
    log_info_hex_u32("Heap virtual end", heap_stats.virtual_end);
    log_info_u32("Heap total KB", heap_stats.total_bytes / 1024);
}

void* kmalloc(uint32_t size)
{
    if (!heap_stats.initialized || size == 0)
    {
        heap_stats.failed_allocation_count++;
        return 0;
    }

    uint32_t requested_size = align_up(size, HEAP_ALIGNMENT);
    heap_block_t* block = heap_head;
    uint32_t visited = 0;

    while (block != 0 && visited < HEAP_MAX_AUDIT_BLOCKS)
    {
        if (!heap_block_header_is_sane(block))
        {
            heap_stats.failed_allocation_count++;
            return 0;
        }

        if (!block->used && block->size >= requested_size)
        {
            heap_split_block(block, requested_size);
            block->used = 1;
            heap_recalculate_stats();

            return (void*)((uint32_t)block + heap_header_size());
        }

        block = block->next;
        visited++;
    }

    if (!heap_grow(requested_size))
    {
        heap_stats.failed_allocation_count++;
        return 0;
    }

    return kmalloc(requested_size);
}

void kfree(void* pointer)
{
    if (pointer == 0)
    {
        return;
    }

    heap_block_t* block = heap_find_block_for_pointer(pointer);

    if (block == 0)
    {
        heap_stats.invalid_free_count++;
        return;
    }

    if (!block->used)
    {
        heap_stats.double_free_count++;
        return;
    }

    block->used = 0;
    heap_stats.successful_free_count++;

    heap_merge_free_blocks();
    heap_recalculate_stats();
}

int heap_validate_pointer(void* pointer)
{
    heap_block_t* block = heap_find_block_for_pointer(pointer);

    if (block == 0)
    {
        return 0;
    }

    return block->used != 0;
}

kernel_status_t heap_validate_pointer_status(void* pointer)
{
    if (pointer == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (!heap_stats.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    uint32_t address = (uint32_t)pointer;

    if (
        address < heap_stats.virtual_start ||
        address >= heap_stats.virtual_end
    )
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (!heap_validate_pointer(pointer))
    {
        return KERNEL_ERROR_CORRUPTION_DETECTED;
    }

    if (!heap_is_pointer_allocated(pointer))
    {
        return KERNEL_ERROR_INVALID_STATE;
    }

    return KERNEL_OK;
}

int heap_is_pointer_allocated(void* pointer)
{
    return heap_validate_pointer(pointer);
}

uint32_t heap_audit(void)
{
    heap_audit_info.audits_run++;
    heap_clear_audit_snapshot();

    if (!heap_stats.initialized)
    {
        heap_audit_record_issue(&heap_audit_info.out_of_range_blocks);
        return heap_audit_info.issues_found;
    }

    uint32_t header_size = heap_header_size();
    heap_block_t* block = heap_head;
    uint32_t previous_address = 0;
    uint32_t visited = 0;

    while (block != 0)
    {
        if (visited >= HEAP_MAX_AUDIT_BLOCKS)
        {
            heap_audit_record_issue(&heap_audit_info.invalid_next_links);
            break;
        }

        uint32_t block_address = (uint32_t)block;
        heap_audit_info.blocks_seen++;

        if (!heap_address_in_range(block_address))
        {
            heap_audit_record_issue(&heap_audit_info.out_of_range_blocks);
            break;
        }

        if (!heap_address_is_aligned(block_address))
        {
            heap_audit_record_issue(&heap_audit_info.misaligned_blocks);
        }

        if (block->magic != HEAP_BLOCK_MAGIC)
        {
            heap_audit_record_issue(&heap_audit_info.invalid_magic_blocks);
            break;
        }

        if (block->size == 0 || block_address + header_size + block->size - 1 > heap_stats.virtual_end)
        {
            heap_audit_record_issue(&heap_audit_info.invalid_size_blocks);
            break;
        }

        if (previous_address != 0 && block_address <= previous_address)
        {
            heap_audit_record_issue(&heap_audit_info.invalid_next_links);
            break;
        }

        if (block->used)
        {
            heap_audit_info.used_blocks++;
            heap_audit_info.calculated_used_bytes += block->size;
        }
        else
        {
            heap_audit_info.free_blocks++;
            heap_audit_info.calculated_free_bytes += block->size;
        }

        heap_audit_info.calculated_total_payload_bytes += block->size;

        previous_address = block_address;
        block = block->next;
        visited++;
    }

    for (
        uint32_t address = heap_stats.virtual_start;
        address < heap_stats.next_virtual_page;
        address += PMM_PAGE_SIZE
    )
    {
        if (!vmm_is_mapped(address))
        {
            heap_audit_record_issue(&heap_audit_info.unmapped_heap_pages);
        }
    }

    if (heap_audit_info.calculated_used_bytes != heap_stats.used_bytes)
    {
        heap_audit_record_issue(0);
    }

    if (heap_audit_info.calculated_free_bytes != heap_stats.free_bytes)
    {
        heap_audit_record_issue(0);
    }

    return heap_audit_info.issues_found;
}

const heap_audit_info_t* heap_get_audit_info(void)
{
    return &heap_audit_info;
}

const heap_stats_t* heap_get_stats(void)
{
    return &heap_stats;
}

void heap_print_stats(void)
{
    heap_recalculate_stats();

    log_info("Kernel heap statistics follow");
    log_info_u32("Heap initialized", heap_stats.initialized);
    log_info_hex_u32("Heap virtual start", heap_stats.virtual_start);
    log_info_hex_u32("Heap virtual end", heap_stats.virtual_end);
    log_info_hex_u32("Heap next virtual page", heap_stats.next_virtual_page);
    log_info_u32("Heap total pages", heap_stats.total_pages);
    log_info_u32("Heap total KB", heap_stats.total_bytes / 1024);
    log_info_u32("Heap used bytes", heap_stats.used_bytes);
    log_info_u32("Heap free bytes", heap_stats.free_bytes);
    log_info_u32("Heap allocations", heap_stats.allocation_count);
    log_info_u32("Heap free blocks", heap_stats.free_block_count);
    log_info_u32("Heap grows", heap_stats.grow_count);
    log_info_u32("Heap failed grows", heap_stats.failed_grow_count);
    log_info_u32("Heap failed allocations", heap_stats.failed_allocation_count);
    log_info_u32("Heap successful frees", heap_stats.successful_free_count);
    log_info_u32("Heap invalid frees", heap_stats.invalid_free_count);
    log_info_u32("Heap double frees", heap_stats.double_free_count);
}
