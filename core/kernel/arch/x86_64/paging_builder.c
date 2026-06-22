#include "paging_builder.h"

#define X86_64_PAGE_PRESENT 0x001ULL
#define X86_64_PAGE_WRITABLE 0x002ULL
#define X86_64_PAGE_HUGE 0x080ULL
#define X86_64_TABLE_FLAGS (X86_64_PAGE_PRESENT | X86_64_PAGE_WRITABLE)
#define X86_64_HUGE_PAGE_FLAGS (X86_64_TABLE_FLAGS | X86_64_PAGE_HUGE)
#define X86_64_PAGE_MASK (X86_64_PAGE_SIZE - 1ULL)

static u64 builder_pml4[X86_64_PAGING_BUILDER_ENTRIES] __attribute__((aligned(4096)));
static u64 builder_identity_pdpt[X86_64_PAGING_BUILDER_ENTRIES] __attribute__((aligned(4096)));
static u64 builder_identity_pd[X86_64_PAGING_BUILDER_ENTRIES] __attribute__((aligned(4096)));
static u64 builder_direct_pdpt[X86_64_PAGING_BUILDER_ENTRIES] __attribute__((aligned(4096)));
static u64 builder_direct_pd[X86_64_PAGING_BUILDER_ENTRIES] __attribute__((aligned(4096)));
static u64 builder_kernel_pdpt[X86_64_PAGING_BUILDER_ENTRIES] __attribute__((aligned(4096)));
static u64 builder_kernel_pd[X86_64_PAGING_BUILDER_ENTRIES] __attribute__((aligned(4096)));
static u64 builder_kernel_pt[X86_64_PAGING_BUILDER_ENTRIES] __attribute__((aligned(4096)));

static void clear_table(u64 *table)
{
    for (u32 i = 0; i < X86_64_PAGING_BUILDER_ENTRIES; ++i) {
        table[i] = 0ULL;
    }
}

static u64 read_cr3(void)
{
    u64 value;
    __asm__ volatile ("movq %%cr3, %0" : "=r" (value));
    return value;
}

static void write_cr3(u64 value)
{
    __asm__ volatile ("movq %0, %%cr3" :: "r" (value) : "memory");
}

static u32 is_page_aligned(u64 value)
{
    return ((value & X86_64_PAGE_MASK) == 0ULL) ? 1U : 0U;
}

static u32 pdpt_index_for(u64 address)
{
    return (u32)((address >> 30) & 0x1FFULL);
}

static u32 pd_index_for(u64 address)
{
    return (u32)((address >> 21) & 0x1FFULL);
}

static u32 align_page_count(u64 bytes)
{
    return (u32)((bytes + X86_64_PAGE_MASK) / X86_64_PAGE_SIZE);
}

static u32 huge_page_count_for(u64 bytes)
{
    return (u32)((bytes + X86_64_HUGE_PAGE_SIZE - 1ULL) / X86_64_HUGE_PAGE_SIZE);
}

static u32 count_present_entries(const u64 *table)
{
    u32 count = 0U;

    for (u32 i = 0; i < X86_64_PAGING_BUILDER_ENTRIES; ++i) {
        if ((table[i] & X86_64_PAGE_PRESENT) != 0ULL) {
            ++count;
        }
    }

    return count;
}

static u32 tables_are_aligned(void)
{
    return ((is_page_aligned((u64)builder_pml4) != 0U) &&
            (is_page_aligned((u64)builder_identity_pdpt) != 0U) &&
            (is_page_aligned((u64)builder_identity_pd) != 0U) &&
            (is_page_aligned((u64)builder_direct_pdpt) != 0U) &&
            (is_page_aligned((u64)builder_direct_pd) != 0U) &&
            (is_page_aligned((u64)builder_kernel_pdpt) != 0U) &&
            (is_page_aligned((u64)builder_kernel_pd) != 0U) &&
            (is_page_aligned((u64)builder_kernel_pt) != 0U)) ? 1U : 0U;
}

void x86_64_paging_builder_init(struct x86_64_paging_builder_state *state,
                                const struct x86_64_memory_layout *layout,
                                const struct x86_64_paging_plan *plan)
{
    clear_table(builder_pml4);
    clear_table(builder_identity_pdpt);
    clear_table(builder_identity_pd);
    clear_table(builder_direct_pdpt);
    clear_table(builder_direct_pd);
    clear_table(builder_kernel_pdpt);
    clear_table(builder_kernel_pd);
    clear_table(builder_kernel_pt);

    u32 identity_pages = huge_page_count_for(plan->identity_window_bytes);
    if (identity_pages > X86_64_PAGING_BUILDER_ENTRIES) {
        identity_pages = X86_64_PAGING_BUILDER_ENTRIES;
    }

    for (u32 i = 0; i < identity_pages; ++i) {
        builder_identity_pd[i] = ((u64)i * X86_64_HUGE_PAGE_SIZE) | X86_64_HUGE_PAGE_FLAGS;
    }

    u32 direct_pages = huge_page_count_for(plan->direct_map_bytes);
    if (direct_pages > X86_64_PAGING_BUILDER_ENTRIES) {
        direct_pages = X86_64_PAGING_BUILDER_ENTRIES;
    }

    for (u32 i = 0; i < direct_pages; ++i) {
        builder_direct_pd[i] = ((u64)i * X86_64_HUGE_PAGE_SIZE) | X86_64_HUGE_PAGE_FLAGS;
    }

    u32 kernel_pages = align_page_count(layout->kernel_size_bytes);
    u32 kernel_pages_fit = (kernel_pages <= X86_64_PAGING_BUILDER_ENTRIES) ? 1U : 0U;
    if (kernel_pages > X86_64_PAGING_BUILDER_ENTRIES) {
        kernel_pages = X86_64_PAGING_BUILDER_ENTRIES;
    }

    for (u32 i = 0; i < kernel_pages; ++i) {
        builder_kernel_pt[i] = (layout->kernel_start + ((u64)i * X86_64_PAGE_SIZE)) | X86_64_TABLE_FLAGS;
    }

    u32 kernel_pdpt_index = pdpt_index_for(plan->kernel_virtual_start);
    u32 kernel_pd_index = pd_index_for(plan->kernel_virtual_start);

    builder_identity_pdpt[0] = ((u64)builder_identity_pd) | X86_64_TABLE_FLAGS;
    builder_direct_pdpt[0] = ((u64)builder_direct_pd) | X86_64_TABLE_FLAGS;
    builder_kernel_pdpt[kernel_pdpt_index] = ((u64)builder_kernel_pd) | X86_64_TABLE_FLAGS;
    builder_kernel_pd[kernel_pd_index] = ((u64)builder_kernel_pt) | X86_64_TABLE_FLAGS;

    builder_pml4[plan->identity_pml4_index] = ((u64)builder_identity_pdpt) | X86_64_TABLE_FLAGS;
    builder_pml4[plan->direct_map_pml4_index] = ((u64)builder_direct_pdpt) | X86_64_TABLE_FLAGS;
    builder_pml4[plan->kernel_pml4_index] = ((u64)builder_kernel_pdpt) | X86_64_TABLE_FLAGS;

    state->pml4_table = (u64)builder_pml4;
    state->identity_pdpt_table = (u64)builder_identity_pdpt;
    state->identity_pd_table = (u64)builder_identity_pd;
    state->direct_pdpt_table = (u64)builder_direct_pdpt;
    state->direct_pd_table = (u64)builder_direct_pd;
    state->kernel_pdpt_table = (u64)builder_kernel_pdpt;
    state->kernel_pd_table = (u64)builder_kernel_pd;
    state->kernel_pt_table = (u64)builder_kernel_pt;
    state->pml4_present_entries = count_present_entries(builder_pml4);
    state->identity_huge_pages = identity_pages;
    state->direct_huge_pages = direct_pages;
    state->kernel_pages = kernel_pages;
    state->identity_entry_ok =
        ((builder_pml4[plan->identity_pml4_index] & X86_64_PAGE_PRESENT) != 0ULL &&
         (builder_identity_pdpt[0] & X86_64_PAGE_PRESENT) != 0ULL &&
         identity_pages == X86_64_PAGING_BUILDER_ENTRIES) ? 1U : 0U;
    state->direct_map_entry_ok =
        ((builder_pml4[plan->direct_map_pml4_index] & X86_64_PAGE_PRESENT) != 0ULL &&
         (builder_direct_pdpt[0] & X86_64_PAGE_PRESENT) != 0ULL &&
         direct_pages == X86_64_PAGING_BUILDER_ENTRIES) ? 1U : 0U;
    state->kernel_entry_ok =
        ((builder_pml4[plan->kernel_pml4_index] & X86_64_PAGE_PRESENT) != 0ULL &&
         (builder_kernel_pdpt[kernel_pdpt_index] & X86_64_PAGE_PRESENT) != 0ULL &&
         (builder_kernel_pd[kernel_pd_index] & X86_64_PAGE_PRESENT) != 0ULL &&
         (builder_kernel_pt[0] & X86_64_PAGE_PRESENT) != 0ULL &&
         (is_page_aligned(layout->kernel_start) != 0U) &&
         (kernel_pages_fit != 0U)) ? 1U : 0U;
    state->tables_aligned = tables_are_aligned();
    state->builder_ok =
        ((state->pml4_present_entries == X86_64_PLANNED_REGION_COUNT) &&
         (state->identity_entry_ok != 0U) &&
         (state->direct_map_entry_ok != 0U) &&
         (state->kernel_entry_ok != 0U) &&
         (state->tables_aligned != 0U) &&
         (plan->plan_ok != 0U)) ? 1U : 0U;
}

void x86_64_paging_builder_activate(struct x86_64_paging_activation_state *state,
                                    const struct x86_64_paging_builder_state *builder)
{
    state->previous_cr3 = read_cr3();
    state->requested_cr3 = builder->pml4_table;
    state->builder_ready = builder->builder_ok;

    if (state->builder_ready != 0U) {
        write_cr3(state->requested_cr3);
    }

    state->active_cr3 = read_cr3();
    state->cr3_changed = (state->active_cr3 != state->previous_cr3) ? 1U : 0U;
    state->active_matches_builder = (state->active_cr3 == state->requested_cr3) ? 1U : 0U;
    state->activation_ok =
        ((state->builder_ready != 0U) &&
         (state->active_matches_builder != 0U)) ? 1U : 0U;
}
