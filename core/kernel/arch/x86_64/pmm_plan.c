#include "pmm_plan.h"

static u64 align_down(u64 value, u64 alignment)
{
    return value & ~(alignment - 1ULL);
}

static u64 align_up(u64 value, u64 alignment)
{
    return align_down(value + alignment - 1ULL, alignment);
}

static void zero_plan(struct x86_64_pmm_plan *plan)
{
    u8 *bytes = (u8 *)plan;
    for (usize i = 0; i < sizeof(*plan); ++i) {
        bytes[i] = 0U;
    }
}

static void add_managed_range(u64 start, u64 end, struct x86_64_pmm_plan *plan)
{
    if (end <= start) {
        return;
    }

    u64 pages = (end - start) / X86_64_PAGE_SIZE;
    if (pages == 0ULL) {
        return;
    }

    if (plan->first_free_page == 0ULL || start < plan->first_free_page) {
        plan->first_free_page = start;
    }

    ++plan->usable_region_count;
    plan->managed_pages += pages;
    plan->managed_bytes += pages * X86_64_PAGE_SIZE;
}

void x86_64_pmm_plan_init(
    const struct x86_64_boot_summary *boot_info,
    const struct x86_64_memory_layout *layout,
    struct x86_64_pmm_plan *plan)
{
    zero_plan(plan);

    u64 reserved_below = align_up(layout->kernel_end, X86_64_PAGE_SIZE);
    plan->reserved_below = reserved_below;

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
        add_managed_range(start, region_end, plan);
    }
}
