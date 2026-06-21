#ifndef LIAM_OS_X86_64_PMM_H
#define LIAM_OS_X86_64_PMM_H

#include "boot_info.h"
#include "memory_layout.h"
#include "types.h"

#define X86_64_PMM_MAX_TRACKED_PAGES 65536U
#define X86_64_PMM_INVALID_PAGE 0ULL

struct x86_64_pmm_state {
    u32 initialized;
    u32 tracked_pages;
    u32 free_pages;
    u32 dropped_pages;
    u32 duplicate_free_rejects;
    u64 lowest_page;
    u64 highest_page;
};

void x86_64_pmm_init(
    const struct x86_64_boot_summary *boot_info,
    const struct x86_64_memory_layout *layout);

u64 x86_64_pmm_alloc_page(void);
u32 x86_64_pmm_free_page(u64 page);
const struct x86_64_pmm_state *x86_64_pmm_get_state(void);

#endif
