#ifndef LIAM_OS_HEAP_H
#define LIAM_OS_HEAP_H

#include "../core/types.h"

#include "../core/status.h"


typedef struct
{
    uint8_t initialized;

    uint32_t virtual_start;
    uint32_t virtual_end;
    uint32_t next_virtual_page;

    uint32_t total_pages;
    uint32_t total_bytes;
    uint32_t used_bytes;
    uint32_t free_bytes;
    uint32_t allocation_count;
    uint32_t free_block_count;

    uint32_t grow_count;
    uint32_t failed_grow_count;
    uint32_t failed_allocation_count;

    uint32_t successful_free_count;
    uint32_t invalid_free_count;
    uint32_t double_free_count;
} heap_stats_t;

typedef struct
{
    uint32_t audits_run;
    uint32_t issues_found;

    uint32_t blocks_seen;
    uint32_t used_blocks;
    uint32_t free_blocks;

    uint32_t invalid_magic_blocks;
    uint32_t out_of_range_blocks;
    uint32_t misaligned_blocks;
    uint32_t invalid_size_blocks;
    uint32_t invalid_next_links;
    uint32_t unmapped_heap_pages;

    uint32_t calculated_used_bytes;
    uint32_t calculated_free_bytes;
    uint32_t calculated_total_payload_bytes;
} heap_audit_info_t;

void heap_initialize(void);
void* kmalloc(uint32_t size);
void kfree(void* pointer);

int heap_validate_pointer(void* pointer);
kernel_status_t heap_validate_pointer_status(void* pointer);
int heap_is_pointer_allocated(void* pointer);
uint32_t heap_audit(void);
const heap_audit_info_t* heap_get_audit_info(void);

const heap_stats_t* heap_get_stats(void);
void heap_print_stats(void);

#endif
