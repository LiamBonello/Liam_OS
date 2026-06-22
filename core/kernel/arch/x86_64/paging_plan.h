#ifndef LIAM_OS_X86_64_PAGING_PLAN_H
#define LIAM_OS_X86_64_PAGING_PLAN_H

#include "memory_layout.h"
#include "pmm_plan.h"
#include "types.h"

#define X86_64_KERNEL_VIRTUAL_BASE 0xFFFFFFFF80000000ULL
#define X86_64_DIRECT_MAP_BASE 0xFFFF800000000000ULL
#define X86_64_TRANSITION_IDENTITY_PML4_INDEX 0U
#define X86_64_PLANNED_REGION_COUNT 3U

struct x86_64_paging_plan {
    u64 identity_window_bytes;
    u64 kernel_virtual_base;
    u64 kernel_virtual_start;
    u64 kernel_virtual_end;
    u64 kernel_virtual_offset;
    u64 direct_map_base;
    u64 direct_map_end;
    u64 direct_map_bytes;
    u32 identity_pml4_index;
    u32 kernel_pml4_index;
    u32 direct_map_pml4_index;
    u32 planned_region_count;
    u32 identity_window_ok;
    u32 kernel_window_canonical;
    u32 direct_map_canonical;
    u32 pml4_slots_distinct;
    u32 plan_ok;
};

void x86_64_paging_plan_init(struct x86_64_paging_plan *plan,
                             const struct x86_64_memory_layout *layout,
                             const struct x86_64_pmm_plan *pmm_plan);

#endif
