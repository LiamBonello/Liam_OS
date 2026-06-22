#include "runtime.h"

typedef u32 (*x86_64_runtime_entry_fn)(struct x86_64_runtime_entry_state *, u64);

static u64 read_cr3(void)
{
    u64 value;
    __asm__ volatile ("movq %%cr3, %0" : "=r" (value));
    return value;
}

static u32 address_in_kernel_image(u64 address, const struct x86_64_memory_layout *layout)
{
    if ((address >= layout->kernel_start) && (address < layout->kernel_end)) {
        return 1U;
    }

    return 0U;
}

__attribute__((noinline))
u32 x86_64_runtime_higher_half_entry(struct x86_64_runtime_entry_state *state, u64 arg)
{
    volatile u32 stack_marker = X86_64_RUNTIME_ENTRY_EXPECTED;

    if (state == (struct x86_64_runtime_entry_state *)0) {
        return 0U;
    }

    state->entered_value = X86_64_RUNTIME_ENTRY_EXPECTED;
    state->scratch_value = stack_marker;
    state->active_cr3 = read_cr3();
    state->arg_value = arg;
    state->stack_sample = (u64)&stack_marker;

    if (arg != X86_64_RUNTIME_ENTRY_ARG) {
        return 0U;
    }

    return X86_64_RUNTIME_ENTRY_EXPECTED;
}

void x86_64_runtime_enter_higher_half(struct x86_64_runtime_entry_state *state,
                                      const struct x86_64_paging_activation_state *activation,
                                      const struct x86_64_memory_layout *layout,
                                      const struct x86_64_paging_plan *plan)
{
    u64 low_entry = (u64)x86_64_runtime_higher_half_entry;
    u64 high_entry = low_entry + plan->kernel_virtual_offset;

    state->low_entry = low_entry;
    state->high_entry = high_entry;
    state->state_pointer = (u64)state;
    state->active_cr3 = 0ULL;
    state->expected_cr3 = activation->requested_cr3;
    state->arg_value = 0ULL;
    state->stack_sample = 0ULL;
    state->expected_value = X86_64_RUNTIME_ENTRY_EXPECTED;
    state->return_value = 0U;
    state->entered_value = 0U;
    state->scratch_value = 0U;
    state->activation_ready = activation->activation_ok;
    state->high_entry_ready = ((state->activation_ready != 0U) &&
                               (address_in_kernel_image(low_entry, layout) != 0U) &&
                               (plan->kernel_window_canonical != 0U)) ? 1U : 0U;

    if (state->high_entry_ready != 0U) {
        x86_64_runtime_entry_fn entry = (x86_64_runtime_entry_fn)high_entry;
        state->return_value = entry(state, X86_64_RUNTIME_ENTRY_ARG);
    }

    state->arg_ok = (state->arg_value == X86_64_RUNTIME_ENTRY_ARG) ? 1U : 0U;
    state->cr3_ok = (state->active_cr3 == state->expected_cr3) ? 1U : 0U;
    state->return_ok = (state->return_value == state->expected_value) ? 1U : 0U;
    state->entered_ok = (state->entered_value == state->expected_value) ? 1U : 0U;
    state->scratch_ok = (state->scratch_value == state->expected_value) ? 1U : 0U;
    state->stack_identity_ok = ((state->stack_sample != 0ULL) &&
                                (state->stack_sample < plan->identity_window_bytes)) ? 1U : 0U;
    state->runtime_ok = ((state->high_entry_ready != 0U) &&
                         (state->arg_ok != 0U) &&
                         (state->cr3_ok != 0U) &&
                         (state->return_ok != 0U) &&
                         (state->entered_ok != 0U) &&
                         (state->scratch_ok != 0U) &&
                         (state->stack_identity_ok != 0U)) ? 1U : 0U;
}
