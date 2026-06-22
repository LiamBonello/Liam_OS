#ifndef LIAM_OS_X86_64_HIGHER_HALF_PROBE_H
#define LIAM_OS_X86_64_HIGHER_HALF_PROBE_H

#include "memory_layout.h"
#include "paging_builder.h"
#include "paging_plan.h"
#include "types.h"

#define X86_64_HIGHER_HALF_PROBE_EXPECTED 0x48485031U
#define X86_64_HIGHER_HALF_C_PROBE_EXPECTED 0x48484331U

struct x86_64_higher_half_probe_state {
    u64 low_entry;
    u64 high_entry;
    u64 entry_bytes;
    u64 c_low_entry;
    u64 c_high_entry;
    u64 c_marker_low;
    u64 c_marker_high;
    u32 expected_value;
    u32 low_result;
    u32 high_result;
    u32 c_expected_value;
    u32 c_low_result;
    u32 c_high_result;
    u32 activation_ready;
    u32 alias_ready;
    u32 low_probe_ok;
    u32 high_probe_ok;
    u32 probe_ok;
    u32 c_alias_ready;
    u32 c_low_probe_ok;
    u32 c_high_probe_ok;
    u32 c_probe_ok;
};

void x86_64_higher_half_probe_run(struct x86_64_higher_half_probe_state *state,
                                  const struct x86_64_paging_activation_state *activation,
                                  const struct x86_64_paging_probe_state *paging_probe,
                                  const struct x86_64_memory_layout *layout,
                                  const struct x86_64_paging_plan *plan);

#endif
