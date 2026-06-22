#ifndef LIAM_OS_X86_64_PAGING_BUILDER_H
#define LIAM_OS_X86_64_PAGING_BUILDER_H

#include "memory_layout.h"
#include "paging_plan.h"
#include "types.h"

#define X86_64_PAGING_BUILDER_ENTRIES 512U

struct x86_64_paging_builder_state {
    u64 pml4_table;
    u64 identity_pdpt_table;
    u64 identity_pd_table;
    u64 direct_pdpt_table;
    u64 direct_pd_table;
    u64 kernel_pdpt_table;
    u64 kernel_pd_table;
    u64 kernel_pt_table;
    u32 pml4_present_entries;
    u32 identity_huge_pages;
    u32 direct_huge_pages;
    u32 kernel_pages;
    u32 identity_entry_ok;
    u32 direct_map_entry_ok;
    u32 kernel_entry_ok;
    u32 tables_aligned;
    u32 builder_ok;
};

void x86_64_paging_builder_init(struct x86_64_paging_builder_state *state,
                                const struct x86_64_memory_layout *layout,
                                const struct x86_64_paging_plan *plan);

#endif
