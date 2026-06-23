#include "tss.h"

struct x86_64_tss {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 reserved3;
    u16 io_map_base;
} __attribute__((packed));

static struct x86_64_tss bootstrap_tss;
static u8 ring0_stack[X86_64_TSS_RING0_STACK_BYTES] __attribute__((aligned(16)));
static u8 double_fault_stack[X86_64_TSS_IST_STACK_BYTES] __attribute__((aligned(16)));
static u8 nmi_stack[X86_64_TSS_IST_STACK_BYTES] __attribute__((aligned(16)));
static u8 page_fault_stack[X86_64_TSS_IST_STACK_BYTES] __attribute__((aligned(16)));

static void zero_tss(void)
{
    u8 *bytes = (u8 *)&bootstrap_tss;
    usize i;

    for (i = 0; i < sizeof(bootstrap_tss); i++) {
        bytes[i] = 0U;
    }
}

static u64 ist_stack_top(u8 *stack)
{
    return (u64)(stack + X86_64_TSS_IST_STACK_BYTES);
}

static u64 ring0_stack_top(void)
{
    return (u64)(ring0_stack + X86_64_TSS_RING0_STACK_BYTES);
}

void x86_64_tss_init(struct x86_64_tss_state *state)
{
    zero_tss();

    bootstrap_tss.rsp0 = ring0_stack_top();
    bootstrap_tss.ist1 = ist_stack_top(double_fault_stack);
    bootstrap_tss.ist2 = ist_stack_top(nmi_stack);
    bootstrap_tss.ist3 = ist_stack_top(page_fault_stack);
    bootstrap_tss.io_map_base = sizeof(bootstrap_tss);

    state->tss_base = (u64)&bootstrap_tss;
    state->tss_limit = (u32)(sizeof(bootstrap_tss) - 1U);
    state->rsp0 = bootstrap_tss.rsp0;
    state->ist1 = bootstrap_tss.ist1;
    state->ist2 = bootstrap_tss.ist2;
    state->ist3 = bootstrap_tss.ist3;
    state->ring0_stack_bytes = X86_64_TSS_RING0_STACK_BYTES;
    state->ist_stack_bytes = X86_64_TSS_IST_STACK_BYTES;
    state->initialized = 1U;
    state->loaded = 0U;
}
