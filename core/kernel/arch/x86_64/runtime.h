#ifndef LIAM_OS_X86_64_RUNTIME_H
#define LIAM_OS_X86_64_RUNTIME_H

#include "memory_layout.h"
#include "paging_builder.h"
#include "paging_plan.h"
#include "types.h"

#define X86_64_RUNTIME_ENTRY_ARG 0x52544E5452493031ULL
#define X86_64_RUNTIME_ENTRY_EXPECTED 0x52543130U

struct x86_64_runtime_entry_state {
    u64 low_entry;
    u64 high_entry;
    u64 state_pointer;
    u64 active_cr3;
    u64 expected_cr3;
    u64 arg_value;
    u64 stack_sample;
    u32 expected_value;
    u32 return_value;
    u32 entered_value;
    u32 scratch_value;
    u32 activation_ready;
    u32 high_entry_ready;
    u32 arg_ok;
    u32 cr3_ok;
    u32 return_ok;
    u32 entered_ok;
    u32 scratch_ok;
    u32 stack_identity_ok;
    u32 runtime_ok;
};

void x86_64_runtime_enter_higher_half(struct x86_64_runtime_entry_state *state,
                                      const struct x86_64_paging_activation_state *activation,
                                      const struct x86_64_memory_layout *layout,
                                      const struct x86_64_paging_plan *plan);

#endif
