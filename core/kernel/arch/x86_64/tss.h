#ifndef LIAM_OS_X86_64_TSS_H
#define LIAM_OS_X86_64_TSS_H

#include "types.h"

#define X86_64_TSS_RING0_STACK_BYTES 16384ULL
#define X86_64_TSS_IST_STACK_BYTES 4096ULL
#define X86_64_TSS_IST_COUNT 3U
#define X86_64_TSS_EXPECTED_LIMIT 103U

struct x86_64_tss_state {
    u64 tss_base;
    u32 tss_limit;
    u64 rsp0;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ring0_stack_bytes;
    u64 ist_stack_bytes;
    u32 initialized;
    u32 loaded;
};

void x86_64_tss_init(struct x86_64_tss_state *state);

#endif
