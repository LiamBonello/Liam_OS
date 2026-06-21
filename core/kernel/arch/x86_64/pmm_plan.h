#ifndef LIAM_OS_X86_64_PMM_PLAN_H
#define LIAM_OS_X86_64_PMM_PLAN_H

#include "boot_info.h"
#include "memory_layout.h"
#include "types.h"

struct x86_64_pmm_plan {
    u32 usable_region_count;
    u64 first_free_page;
    u64 managed_pages;
    u64 managed_bytes;
    u64 reserved_below;
};

void x86_64_pmm_plan_init(
    const struct x86_64_boot_summary *boot_info,
    const struct x86_64_memory_layout *layout,
    struct x86_64_pmm_plan *plan);

#endif
