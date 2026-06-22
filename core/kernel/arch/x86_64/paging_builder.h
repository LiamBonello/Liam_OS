#ifndef LIAM_OS_X86_64_PAGING_BUILDER_H
#define LIAM_OS_X86_64_PAGING_BUILDER_H

#include "memory_layout.h"
#include "paging_plan.h"
#include "types.h"

#define X86_64_PAGING_BUILDER_ENTRIES 512U
#define X86_64_PAGING_BUILDER_TABLE_PAGES 8U

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
    u32 pmm_backed;
    u32 allocated_table_pages;
    u32 pmm_free_pages_before;
    u32 pmm_free_pages_after;
    u32 allocation_ok;
    u32 identity_entry_ok;
    u32 direct_map_entry_ok;
    u32 kernel_entry_ok;
    u32 tables_aligned;
    u32 builder_ok;
};

struct x86_64_paging_activation_state {
    u64 previous_cr3;
    u64 requested_cr3;
    u64 active_cr3;
    u32 builder_ready;
    u32 cr3_changed;
    u32 active_matches_builder;
    u32 activation_ok;
};

struct x86_64_paging_probe_state {
    u64 identity_addr;
    u64 direct_map_addr;
    u64 kernel_alias_addr;
    u32 identity_value;
    u32 direct_map_value;
    u32 kernel_alias_value;
    u32 activation_ready;
    u32 identity_probe_ok;
    u32 direct_map_probe_ok;
    u32 kernel_alias_probe_ok;
    u32 probes_ok;
};

void x86_64_paging_builder_init(struct x86_64_paging_builder_state *state,
                                const struct x86_64_memory_layout *layout,
                                const struct x86_64_paging_plan *plan);
void x86_64_paging_builder_activate(struct x86_64_paging_activation_state *state,
                                    const struct x86_64_paging_builder_state *builder);
void x86_64_paging_probe_active_mappings(struct x86_64_paging_probe_state *state,
                                         const struct x86_64_paging_activation_state *activation,
                                         const struct x86_64_memory_layout *layout,
                                         const struct x86_64_paging_plan *plan);

#endif
