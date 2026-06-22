#include "paging_plan.h"

static u32 pml4_index_for(u64 address)
{
    return (u32)((address >> 39) & 0x1FFULL);
}

static u32 is_canonical_address(u64 address)
{
    u64 high = address >> 48;
    u64 sign = (address >> 47) & 1ULL;

    if (sign != 0ULL) {
        return (high == 0xFFFFULL) ? 1U : 0U;
    }

    return (high == 0ULL) ? 1U : 0U;
}

static u64 last_byte_for_span(u64 base, u64 bytes)
{
    if (bytes == 0ULL) {
        return base;
    }

    return base + bytes - 1ULL;
}

void x86_64_paging_plan_init(struct x86_64_paging_plan *plan,
                             const struct x86_64_memory_layout *layout,
                             const struct x86_64_pmm_plan *pmm_plan)
{
    plan->identity_window_bytes = layout->bootstrap_identity_map_bytes;
    plan->kernel_virtual_base = X86_64_KERNEL_VIRTUAL_BASE;
    plan->kernel_virtual_offset = X86_64_KERNEL_VIRTUAL_BASE - layout->kernel_load_base;
    plan->kernel_virtual_start = layout->kernel_start + plan->kernel_virtual_offset;
    plan->kernel_virtual_end = layout->kernel_end + plan->kernel_virtual_offset;
    plan->direct_map_base = X86_64_DIRECT_MAP_BASE;
    plan->direct_map_bytes = pmm_plan->reserved_below + pmm_plan->managed_bytes;
    if (plan->direct_map_bytes < layout->bootstrap_identity_map_bytes) {
        plan->direct_map_bytes = layout->bootstrap_identity_map_bytes;
    }
    plan->direct_map_end = last_byte_for_span(plan->direct_map_base, plan->direct_map_bytes);

    plan->identity_pml4_index = X86_64_TRANSITION_IDENTITY_PML4_INDEX;
    plan->kernel_pml4_index = pml4_index_for(plan->kernel_virtual_base);
    plan->direct_map_pml4_index = pml4_index_for(plan->direct_map_base);
    plan->planned_region_count = X86_64_PLANNED_REGION_COUNT;

    plan->identity_window_ok =
        (plan->identity_window_bytes >= layout->bootstrap_identity_map_bytes) ? 1U : 0U;
    plan->kernel_window_canonical =
        ((is_canonical_address(plan->kernel_virtual_start) != 0U) &&
         (is_canonical_address(plan->kernel_virtual_end) != 0U)) ? 1U : 0U;
    plan->direct_map_canonical =
        ((is_canonical_address(plan->direct_map_base) != 0U) &&
         (is_canonical_address(plan->direct_map_end) != 0U)) ? 1U : 0U;
    plan->pml4_slots_distinct =
        ((plan->identity_pml4_index != plan->kernel_pml4_index) &&
         (plan->identity_pml4_index != plan->direct_map_pml4_index) &&
         (plan->kernel_pml4_index != plan->direct_map_pml4_index)) ? 1U : 0U;
    plan->plan_ok =
        ((plan->identity_window_ok != 0U) &&
         (plan->kernel_window_canonical != 0U) &&
         (plan->direct_map_canonical != 0U) &&
         (plan->pml4_slots_distinct != 0U) &&
         (plan->planned_region_count == X86_64_PLANNED_REGION_COUNT)) ? 1U : 0U;
}
