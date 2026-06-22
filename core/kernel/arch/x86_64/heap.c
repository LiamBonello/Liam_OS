#include "heap.h"
#include "memory_layout.h"
#include "pmm.h"
#include "process.h"

#define X86_64_HEAP_ALIGNMENT_DEFAULT 16ULL
#define X86_64_HEAP_INVALID_INDEX 0xFFFFFFFFU
#define X86_64_HEAP_PATTERN_A 0xA5U
#define X86_64_HEAP_PATTERN_B 0x5AU

static struct x86_64_heap_state heap_state;
static u64 heap_physical_pages[X86_64_EARLY_HEAP_PAGES];
static u64 heap_virtual_pages[X86_64_EARLY_HEAP_PAGES];
static u64 heap_next_offset;

static void clear_bytes(void *ptr, usize size)
{
    u8 *bytes = (u8 *)ptr;
    for (usize i = 0; i < size; ++i) {
        bytes[i] = 0U;
    }
}

static u64 align_up_u64(u64 value, u64 alignment)
{
    return (value + alignment - 1ULL) & ~(alignment - 1ULL);
}

static u32 is_power_of_two(u64 value)
{
    return ((value != 0ULL) && ((value & (value - 1ULL)) == 0ULL)) ? 1U : 0U;
}

static u32 is_aligned(u64 value, u64 alignment)
{
    return ((value & (alignment - 1ULL)) == 0ULL) ? 1U : 0U;
}

static u32 page_index_for_offset(u64 offset)
{
    u64 index = offset / X86_64_PAGE_SIZE;
    if (index >= X86_64_EARLY_HEAP_PAGES) {
        return X86_64_HEAP_INVALID_INDEX;
    }

    return (u32)index;
}

static void *virtual_for_offset(u64 offset)
{
    u32 page_index = page_index_for_offset(offset);
    if (page_index == X86_64_HEAP_INVALID_INDEX) {
        return (void *)0;
    }

    return (void *)(heap_virtual_pages[page_index] + (offset & (X86_64_PAGE_SIZE - 1ULL)));
}

static void fill_pattern(void *ptr, usize size, u8 pattern)
{
    u8 *bytes = (u8 *)ptr;
    for (usize i = 0; i < size; ++i) {
        bytes[i] = pattern;
    }
}

static u32 verify_pattern(const void *ptr, usize size, u8 pattern)
{
    const u8 *bytes = (const u8 *)ptr;
    for (usize i = 0; i < size; ++i) {
        if (bytes[i] != pattern) {
            return 0U;
        }
    }

    return 1U;
}

static u32 heap_pages_are_available(void)
{
    for (u32 i = 0; i < X86_64_EARLY_HEAP_PAGES; ++i) {
        if (heap_physical_pages[i] == X86_64_PMM_INVALID_PAGE || heap_virtual_pages[i] == 0ULL) {
            return 0U;
        }
    }

    return 1U;
}

static void rollback_heap_pages(u32 allocated_pages)
{
    for (u32 i = 0; i < allocated_pages; ++i) {
        if (heap_physical_pages[i] != X86_64_PMM_INVALID_PAGE) {
            (void)x86_64_pmm_free_page(heap_physical_pages[i]);
            heap_physical_pages[i] = X86_64_PMM_INVALID_PAGE;
            heap_virtual_pages[i] = 0ULL;
        }
    }
}

void x86_64_heap_init(struct x86_64_heap_state *state,
                      const struct x86_64_paging_plan *plan)
{
    clear_bytes(&heap_state, sizeof(heap_state));
    clear_bytes(heap_physical_pages, sizeof(heap_physical_pages));
    clear_bytes(heap_virtual_pages, sizeof(heap_virtual_pages));
    heap_next_offset = 0ULL;

    const struct x86_64_pmm_state *pmm_state = x86_64_pmm_get_state();
    heap_state.pmm_free_pages_before = pmm_state->free_pages;
    heap_state.heap_pages = X86_64_EARLY_HEAP_PAGES;
    heap_state.heap_bytes = (u64)X86_64_EARLY_HEAP_PAGES * X86_64_PAGE_SIZE;

    u32 allocated_pages = 0U;
    for (u32 i = 0; i < X86_64_EARLY_HEAP_PAGES; ++i) {
        u64 page = x86_64_pmm_alloc_page();
        if (page == X86_64_PMM_INVALID_PAGE) {
            rollback_heap_pages(allocated_pages);
            heap_state.pmm_free_pages_after = x86_64_pmm_get_state()->free_pages;
            if (state != &heap_state) {
                *state = heap_state;
            }
            return;
        }

        heap_physical_pages[i] = page;
        heap_virtual_pages[i] = plan->direct_map_base + page;
        clear_bytes((void *)heap_virtual_pages[i], X86_64_PAGE_SIZE);
        allocated_pages += 1U;
    }

    heap_state.pmm_free_pages_after = x86_64_pmm_get_state()->free_pages;
    heap_state.first_physical_page = heap_physical_pages[0];
    heap_state.last_physical_page = heap_physical_pages[X86_64_EARLY_HEAP_PAGES - 1U];
    heap_state.first_virtual_page = heap_virtual_pages[0];
    heap_state.last_virtual_page = heap_virtual_pages[X86_64_EARLY_HEAP_PAGES - 1U];
    heap_state.pmm_backed = heap_pages_are_available();
    heap_state.direct_map_ok = ((plan->direct_map_canonical != 0U) && (plan->direct_map_bytes >= heap_state.heap_bytes)) ? 1U : 0U;
    heap_state.initialized = ((heap_state.pmm_backed != 0U) && (heap_state.direct_map_ok != 0U)) ? 1U : 0U;

    if (state != &heap_state) {
        *state = heap_state;
    }
}

void *x86_64_heap_alloc(usize size, usize alignment)
{
    if (heap_state.initialized == 0U || size == 0ULL) {
        return (void *)0;
    }

    u64 effective_alignment = (alignment == 0ULL) ? X86_64_HEAP_ALIGNMENT_DEFAULT : alignment;
    if (is_power_of_two(effective_alignment) == 0U) {
        return (void *)0;
    }

    u64 aligned_offset = align_up_u64(heap_next_offset, effective_alignment);
    u64 end_offset = aligned_offset + size;
    if (end_offset > heap_state.heap_bytes || end_offset < aligned_offset) {
        return (void *)0;
    }

    void *allocation = virtual_for_offset(aligned_offset);
    if (allocation == (void *)0) {
        return (void *)0;
    }

    heap_next_offset = end_offset;
    heap_state.used_bytes = heap_next_offset;
    heap_state.allocations += 1U;

    return allocation;
}

void x86_64_heap_run_smoke(struct x86_64_heap_state *state)
{
    void *first = x86_64_heap_alloc(64ULL, 16ULL);
    void *second = x86_64_heap_alloc(128ULL, 64ULL);

    heap_state.first_allocation = (u64)first;
    heap_state.second_allocation = (u64)second;
    heap_state.alignment_ok =
        ((first != (void *)0) &&
         (second != (void *)0) &&
         (is_aligned((u64)first, 16ULL) != 0U) &&
         (is_aligned((u64)second, 64ULL) != 0U)) ? 1U : 0U;

    if (first != (void *)0) {
        fill_pattern(first, 64ULL, X86_64_HEAP_PATTERN_A);
    }

    if (second != (void *)0) {
        fill_pattern(second, 128ULL, X86_64_HEAP_PATTERN_B);
    }

    heap_state.pattern_ok =
        ((first != (void *)0) &&
         (second != (void *)0) &&
         (verify_pattern(first, 64ULL, X86_64_HEAP_PATTERN_A) != 0U) &&
         (verify_pattern(second, 128ULL, X86_64_HEAP_PATTERN_B) != 0U)) ? 1U : 0U;

    heap_state.smoke_ok =
        ((heap_state.initialized != 0U) &&
         (heap_state.pmm_backed != 0U) &&
         (heap_state.direct_map_ok != 0U) &&
         (heap_state.alignment_ok != 0U) &&
         (heap_state.pattern_ok != 0U)) ? 1U : 0U;

    if (state != &heap_state) {
        *state = heap_state;
    }

    if (heap_state.smoke_ok != 0U) {
        struct x86_64_process_smoke_state process_smoke;
        x86_64_process_run_smoke(&process_smoke);
    }
}

const struct x86_64_heap_state *x86_64_heap_get_state(void)
{
    return &heap_state;
}
