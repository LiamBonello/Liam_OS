#ifndef LIAM_OS_X86_64_PAGING_BUILDER_H
#define LIAM_OS_X86_64_PAGING_BUILDER_H

#include "boot_info.h"
#include "memory_layout.h"
#include "paging_plan.h"
#include "types.h"

#define X86_64_PAGING_BUILDER_ENTRIES 512U
#define X86_64_FRAMEBUFFER_VIRTUAL_BASE 0xFFFF900000000000ULL
#define X86_64_PAGING_BUILDER_TABLE_PAGES 13U
#define X86_64_PAGING_BUILDER_USER_PHYSICAL_PAGES 2U
#define X86_64_PAGING_BUILDER_ALLOCATED_PAGES \
    (X86_64_PAGING_BUILDER_TABLE_PAGES + X86_64_PAGING_BUILDER_USER_PHYSICAL_PAGES)

struct x86_64_paging_builder_state {
    u64 pml4_table;
    u64 identity_pdpt_table;
    u64 identity_pd_table;
    u64 direct_pdpt_table;
    u64 direct_pd_table;
    u64 framebuffer_pdpt_table;
    u64 framebuffer_pd_table;
    u64 kernel_pdpt_table;
    u64 kernel_pd_table;
    u64 kernel_pt_table;
    u64 user_code_pt_table;
    u64 user_stack_pd_table;
    u64 user_stack_pt_table;
    u64 user_code_page;
    u64 user_stack_page;
    u64 user_code_virtual;
    u64 user_stack_virtual;
    u64 framebuffer_physical_base;
    u64 framebuffer_virtual_base;
    u64 framebuffer_virtual_address;
    u64 framebuffer_bytes;
    u32 framebuffer_huge_pages;
    u32 pml4_present_entries;
    u32 identity_huge_pages;
    u32 direct_huge_pages;
    u32 kernel_pages;
    u32 pmm_backed;
    u32 allocated_table_pages;
    u32 allocated_user_pages;
    u32 pmm_free_pages_before;
    u32 pmm_free_pages_after;
    u32 allocation_ok;
    u32 identity_entry_ok;
    u32 direct_map_entry_ok;
    u32 framebuffer_requested;
    u32 framebuffer_entry_ok;
    u32 framebuffer_mapped;
    u32 kernel_entry_ok;
    u32 user_code_entry_ok;
    u32 user_stack_entry_ok;
    u32 user_pages_user_accessible;
    u32 user_image_embedded;
    u32 user_image_source_ok;
    u32 user_image_bytes;
    u32 user_image_checksum;
    u32 user_image_loaded;
    u32 user_stack_zeroed;
    u32 elf64_header_ok;
    u32 elf64_program_header_ok;
    u32 elf64_entry_ok;
    u32 elf64_segment_ok;
    u32 elf64_load_ok;
    u32 user_mapping_ok;
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
                                const struct x86_64_paging_plan *plan,
                                const struct x86_64_boot_summary *boot_info);
void x86_64_paging_builder_activate(struct x86_64_paging_activation_state *state,
                                    const struct x86_64_paging_builder_state *builder);
void x86_64_paging_probe_active_mappings(struct x86_64_paging_probe_state *state,
                                         const struct x86_64_paging_activation_state *activation,
                                         const struct x86_64_memory_layout *layout,
                                         const struct x86_64_paging_plan *plan);

#endif
