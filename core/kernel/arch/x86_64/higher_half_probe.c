#include "higher_half_probe.h"

typedef u32 (*x86_64_higher_half_probe_fn)(void);
typedef u32 (*x86_64_higher_half_handoff_fn)(u64, volatile u32 *);

extern u32 x86_64_higher_half_probe_entry(void);
extern u8 x86_64_higher_half_probe_entry_end;

static const volatile u32 x86_64_higher_half_c_probe_marker = X86_64_HIGHER_HALF_C_PROBE_EXPECTED;

__attribute__((noinline))
u32 x86_64_higher_half_c_probe_entry(void)
{
    return x86_64_higher_half_c_probe_marker;
}

__attribute__((noinline))
u32 x86_64_higher_half_handoff_entry(u64 expected_arg, volatile u32 *scratch)
{
    if (scratch == (volatile u32 *)0) {
        return 0U;
    }

    *scratch = X86_64_HIGHER_HALF_HANDOFF_EXPECTED;

    if (expected_arg != X86_64_HIGHER_HALF_HANDOFF_ARG) {
        return 0U;
    }

    return *scratch;
}

static u32 x86_64_address_in_kernel_image(u64 address,
                                           const struct x86_64_memory_layout *layout)
{
    if ((address >= layout->kernel_start) && (address < layout->kernel_end)) {
        return 1U;
    }

    return 0U;
}

void x86_64_higher_half_probe_run(struct x86_64_higher_half_probe_state *state,
                                  const struct x86_64_paging_activation_state *activation,
                                  const struct x86_64_paging_probe_state *paging_probe,
                                  const struct x86_64_memory_layout *layout,
                                  const struct x86_64_paging_plan *plan)
{
    volatile u32 handoff_scratch = 0U;
    u64 low_entry = (u64)x86_64_higher_half_probe_entry;
    u64 low_entry_end = (u64)&x86_64_higher_half_probe_entry_end;
    u64 high_entry = low_entry + plan->kernel_virtual_offset;
    u64 c_low_entry = (u64)x86_64_higher_half_c_probe_entry;
    u64 c_high_entry = c_low_entry + plan->kernel_virtual_offset;
    u64 c_marker_low = (u64)&x86_64_higher_half_c_probe_marker;
    u64 c_marker_high = c_marker_low + plan->kernel_virtual_offset;
    u64 handoff_low_entry = (u64)x86_64_higher_half_handoff_entry;
    u64 handoff_high_entry = handoff_low_entry + plan->kernel_virtual_offset;
    x86_64_higher_half_probe_fn low_fn = (x86_64_higher_half_probe_fn)low_entry;
    x86_64_higher_half_probe_fn c_low_fn = (x86_64_higher_half_probe_fn)c_low_entry;

    state->low_entry = low_entry;
    state->high_entry = high_entry;
    state->entry_bytes = (low_entry_end > low_entry) ? (low_entry_end - low_entry) : 0ULL;
    state->c_low_entry = c_low_entry;
    state->c_high_entry = c_high_entry;
    state->c_marker_low = c_marker_low;
    state->c_marker_high = c_marker_high;
    state->handoff_low_entry = handoff_low_entry;
    state->handoff_high_entry = handoff_high_entry;
    state->handoff_scratch_low = (u64)&handoff_scratch;
    state->handoff_arg = X86_64_HIGHER_HALF_HANDOFF_ARG;
    state->expected_value = X86_64_HIGHER_HALF_PROBE_EXPECTED;
    state->low_result = low_fn();
    state->high_result = 0U;
    state->c_expected_value = X86_64_HIGHER_HALF_C_PROBE_EXPECTED;
    state->c_low_result = c_low_fn();
    state->c_high_result = 0U;
    state->handoff_expected_value = X86_64_HIGHER_HALF_HANDOFF_EXPECTED;
    state->handoff_result = 0U;
    state->handoff_scratch_result = 0U;
    state->activation_ready = activation->activation_ok;
    state->alias_ready = ((paging_probe->probes_ok != 0U) &&
                          (x86_64_address_in_kernel_image(low_entry, layout) != 0U) &&
                          (state->entry_bytes != 0ULL)) ? 1U : 0U;
    state->low_probe_ok = (state->low_result == state->expected_value) ? 1U : 0U;

    if ((state->activation_ready != 0U) &&
        (state->alias_ready != 0U) &&
        (state->low_probe_ok != 0U)) {
        x86_64_higher_half_probe_fn high_fn = (x86_64_higher_half_probe_fn)high_entry;
        state->high_result = high_fn();
    }

    state->high_probe_ok = (state->high_result == state->expected_value) ? 1U : 0U;
    state->probe_ok = ((state->activation_ready != 0U) &&
                       (state->alias_ready != 0U) &&
                       (state->low_probe_ok != 0U) &&
                       (state->high_probe_ok != 0U)) ? 1U : 0U;

    state->c_alias_ready = ((state->probe_ok != 0U) &&
                            (x86_64_address_in_kernel_image(c_low_entry, layout) != 0U) &&
                            (x86_64_address_in_kernel_image(c_marker_low, layout) != 0U)) ? 1U : 0U;
    state->c_low_probe_ok = (state->c_low_result == state->c_expected_value) ? 1U : 0U;

    if ((state->c_alias_ready != 0U) && (state->c_low_probe_ok != 0U)) {
        x86_64_higher_half_probe_fn c_high_fn = (x86_64_higher_half_probe_fn)c_high_entry;
        state->c_high_result = c_high_fn();
    }

    state->c_high_probe_ok = (state->c_high_result == state->c_expected_value) ? 1U : 0U;
    state->c_probe_ok = ((state->c_alias_ready != 0U) &&
                         (state->c_low_probe_ok != 0U) &&
                         (state->c_high_probe_ok != 0U)) ? 1U : 0U;

    state->handoff_ready = ((state->c_probe_ok != 0U) &&
                            (x86_64_address_in_kernel_image(handoff_low_entry, layout) != 0U)) ? 1U : 0U;

    if (state->handoff_ready != 0U) {
        x86_64_higher_half_handoff_fn handoff_fn =
            (x86_64_higher_half_handoff_fn)handoff_high_entry;
        state->handoff_result = handoff_fn(state->handoff_arg, &handoff_scratch);
        state->handoff_scratch_result = handoff_scratch;
    }

    state->handoff_result_ok = (state->handoff_result == state->handoff_expected_value) ? 1U : 0U;
    state->handoff_scratch_ok = (state->handoff_scratch_result == state->handoff_expected_value) ? 1U : 0U;
    state->handoff_ok = ((state->handoff_ready != 0U) &&
                         (state->handoff_result_ok != 0U) &&
                         (state->handoff_scratch_ok != 0U)) ? 1U : 0U;
}
