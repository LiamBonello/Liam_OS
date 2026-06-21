#include "pmm.h"

static u64 page_stack[X86_64_PMM_MAX_TRACKED_PAGES];
static struct x86_64_pmm_state pmm_state;

static u64 align_down(u64 value, u64 alignment)
{
    return value & ~(alignment - 1ULL);
}

static u64 align_up(u64 value, u64 alignment)
{
    return align_down(value + alignment - 1ULL, alignment);
}

static void zero_state(void)
{
    u8 *bytes = (u8 *)&pmm_state;
    for (usize i = 0; i < sizeof(pmm_state); ++i) {
        bytes[i] = 0U;
    }
}

static void track_page(u64 page)
{
    if (pmm_state.tracked_pages >= X86_64_PMM_MAX_TRACKED_PAGES) {
        ++pmm_state.dropped_pages;
        return;
    }

    page_stack[pmm_state.free_pages++] = page;
    ++pmm_state.tracked_pages;

    if (pmm_state.lowest_page == 0ULL || page < pmm_state.lowest_page) {
        pmm_state.lowest_page = page;
    }

    if (page > pmm_state.highest_page) {
        pmm_state.highest_page = page;
    }
}

static void track_range(u64 start, u64 end)
{
    if (end <= start || end - start < X86_64_PAGE_SIZE) {
        return;
    }

    for (u64 page = start; page <= end - X86_64_PAGE_SIZE; page += X86_64_PAGE_SIZE) {
        track_page(page);
    }
}

static u32 page_is_already_free(u64 page)
{
    for (u32 i = 0; i < pmm_state.free_pages; ++i) {
        if (page_stack[i] == page) {
            return 1U;
        }
    }

    return 0U;
}

void x86_64_pmm_init(
    const struct x86_64_boot_summary *boot_info,
    const struct x86_64_memory_layout *layout)
{
    zero_state();

    u64 reserved_below = align_up(layout->kernel_end, X86_64_PAGE_SIZE);

    for (u32 i = 0; i < boot_info->memory_region_count; ++i) {
        const struct x86_64_memory_region *region = &boot_info->memory_regions[i];

        if (region->type != X86_64_MEMORY_REGION_AVAILABLE) {
            continue;
        }

        u64 region_end = region->base + region->length;
        if (region_end < region->base || region_end <= reserved_below) {
            continue;
        }

        u64 start = region->base;
        if (start < reserved_below) {
            start = reserved_below;
        }

        start = align_up(start, X86_64_PAGE_SIZE);
        region_end = align_down(region_end, X86_64_PAGE_SIZE);
        track_range(start, region_end);
    }

    pmm_state.initialized = 1U;
}

u64 x86_64_pmm_alloc_page(void)
{
    if (pmm_state.initialized == 0U || pmm_state.free_pages == 0U) {
        return X86_64_PMM_INVALID_PAGE;
    }

    return page_stack[--pmm_state.free_pages];
}

u32 x86_64_pmm_free_page(u64 page)
{
    if (pmm_state.initialized == 0U || page == X86_64_PMM_INVALID_PAGE) {
        return 0U;
    }

    if ((page & (X86_64_PAGE_SIZE - 1ULL)) != 0ULL) {
        return 0U;
    }

    if (page < pmm_state.lowest_page || page > pmm_state.highest_page) {
        return 0U;
    }

    if (page_is_already_free(page) != 0U) {
        ++pmm_state.duplicate_free_rejects;
        return 0U;
    }

    if (pmm_state.free_pages >= pmm_state.tracked_pages) {
        return 0U;
    }

    page_stack[pmm_state.free_pages++] = page;
    return 1U;
}

const struct x86_64_pmm_state *x86_64_pmm_get_state(void)
{
    return &pmm_state;
}
