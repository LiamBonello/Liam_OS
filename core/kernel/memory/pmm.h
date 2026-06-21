#ifndef LIAM_OS_PMM_H
#define LIAM_OS_PMM_H

#include "../core/types.h"

#include "../core/boot.h"


#define PMM_PAGE_SIZE 4096
#define PMM_MAX_FREE_PAGES 1024

typedef struct {
    uint32_t total_regions;
    uint32_t usable_regions;
    uint32_t reserved_regions;
    uint64_t usable_memory_bytes;

    uint32_t allocation_start;
    uint32_t allocation_end;
    uint32_t next_free_page;
    uint32_t allocated_pages;

    uint32_t free_stack_count;
} pmm_stats_t;

void pmm_initialize(const boot_info_t* boot_info);
const pmm_stats_t* pmm_get_stats(void);
void pmm_print_stats(void);

void* pmm_alloc_page(void);
void pmm_free_page(void* page);
void pmm_run_self_test(void);

#endif