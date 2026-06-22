#include "cpuid.h"
#include "syscall.h"

#define CPUID_BASIC_INFO 0x00000000U
#define CPUID_BASIC_FEATURES 0x00000001U
#define CPUID_EXTENDED_INFO 0x80000000U
#define CPUID_EXTENDED_FEATURES 0x80000001U

#define CPUID_FEATURE_EDX_FPU (1U << 0U)
#define CPUID_FEATURE_EDX_MSR (1U << 5U)
#define CPUID_FEATURE_EDX_APIC (1U << 9U)
#define CPUID_FEATURE_EDX_SSE (1U << 25U)
#define CPUID_FEATURE_EDX_SSE2 (1U << 26U)
#define CPUID_FEATURE_ECX_HYPERVISOR (1U << 31U)

#define CPUID_EXT_FEATURE_EDX_FAST_SYSCALL (1U << 11U)
#define CPUID_EXT_FEATURE_EDX_NX (1U << 20U)
#define CPUID_EXT_FEATURE_EDX_1G_PAGES (1U << 26U)
#define CPUID_EXT_FEATURE_EDX_LONG_MODE (1U << 29U)

struct cpuid_regs {
    u32 eax;
    u32 ebx;
    u32 ecx;
    u32 edx;
};

static void zero_state(struct x86_64_cpuid_state *state)
{
    u8 *bytes = (u8 *)state;
    for (usize i = 0; i < sizeof(*state); ++i) {
        bytes[i] = 0U;
    }
}

static void cpuid_leaf(u32 leaf, u32 subleaf, struct cpuid_regs *regs)
{
    __asm__ volatile (
        "cpuid"
        : "=a" (regs->eax), "=b" (regs->ebx), "=c" (regs->ecx), "=d" (regs->edx)
        : "a" (leaf), "c" (subleaf)
    );
}

static u32 cpuid_available(void)
{
    u64 flags_before;
    u64 flags_after;
    u64 toggled_flags;
    const u64 id_bit = 1ULL << 21U;

    __asm__ volatile ("pushfq; popq %0" : "=r" (flags_before));
    toggled_flags = flags_before ^ id_bit;
    __asm__ volatile ("pushq %0; popfq" : : "r" (toggled_flags) : "cc");
    __asm__ volatile ("pushfq; popq %0" : "=r" (flags_after));
    __asm__ volatile ("pushq %0; popfq" : : "r" (flags_before) : "cc");

    return (((flags_before ^ flags_after) & id_bit) != 0U) ? 1U : 0U;
}

static void copy_vendor(struct x86_64_cpuid_state *state, const struct cpuid_regs *regs)
{
    state->vendor[0] = (char)(regs->ebx & 0xFFU);
    state->vendor[1] = (char)((regs->ebx >> 8U) & 0xFFU);
    state->vendor[2] = (char)((regs->ebx >> 16U) & 0xFFU);
    state->vendor[3] = (char)((regs->ebx >> 24U) & 0xFFU);

    state->vendor[4] = (char)(regs->edx & 0xFFU);
    state->vendor[5] = (char)((regs->edx >> 8U) & 0xFFU);
    state->vendor[6] = (char)((regs->edx >> 16U) & 0xFFU);
    state->vendor[7] = (char)((regs->edx >> 24U) & 0xFFU);

    state->vendor[8] = (char)(regs->ecx & 0xFFU);
    state->vendor[9] = (char)((regs->ecx >> 8U) & 0xFFU);
    state->vendor[10] = (char)((regs->ecx >> 16U) & 0xFFU);
    state->vendor[11] = (char)((regs->ecx >> 24U) & 0xFFU);
    state->vendor[12] = '\0';
}

static void report_syscall_abi_plan(const struct x86_64_cpuid_state *state)
{
    struct x86_64_syscall_abi_state syscall_abi;

    x86_64_syscall_abi_init(&syscall_abi, state->has_fast_syscall);
    x86_64_syscall_abi_report(&syscall_abi);
}

void x86_64_cpuid_state_init(struct x86_64_cpuid_state *state)
{
    struct cpuid_regs regs;

    zero_state(state);
    state->cpuid_available = cpuid_available();
    if (state->cpuid_available == 0U) {
        return;
    }

    cpuid_leaf(CPUID_BASIC_INFO, 0U, &regs);
    state->max_basic_leaf = regs.eax;
    copy_vendor(state, &regs);

    if (state->max_basic_leaf >= CPUID_BASIC_FEATURES) {
        cpuid_leaf(CPUID_BASIC_FEATURES, 0U, &regs);
        state->has_fpu = ((regs.edx & CPUID_FEATURE_EDX_FPU) != 0U) ? 1U : 0U;
        state->has_msr = ((regs.edx & CPUID_FEATURE_EDX_MSR) != 0U) ? 1U : 0U;
        state->has_apic = ((regs.edx & CPUID_FEATURE_EDX_APIC) != 0U) ? 1U : 0U;
        state->has_sse = ((regs.edx & CPUID_FEATURE_EDX_SSE) != 0U) ? 1U : 0U;
        state->has_sse2 = ((regs.edx & CPUID_FEATURE_EDX_SSE2) != 0U) ? 1U : 0U;
        state->hypervisor_present = ((regs.ecx & CPUID_FEATURE_ECX_HYPERVISOR) != 0U) ? 1U : 0U;
    }

    cpuid_leaf(CPUID_EXTENDED_INFO, 0U, &regs);
    state->max_extended_leaf = regs.eax;

    if (state->max_extended_leaf >= CPUID_EXTENDED_FEATURES) {
        cpuid_leaf(CPUID_EXTENDED_FEATURES, 0U, &regs);
        state->has_fast_syscall = ((regs.edx & CPUID_EXT_FEATURE_EDX_FAST_SYSCALL) != 0U) ? 1U : 0U;
        state->has_nx = ((regs.edx & CPUID_EXT_FEATURE_EDX_NX) != 0U) ? 1U : 0U;
        state->has_1g_pages = ((regs.edx & CPUID_EXT_FEATURE_EDX_1G_PAGES) != 0U) ? 1U : 0U;
        state->has_long_mode = ((regs.edx & CPUID_EXT_FEATURE_EDX_LONG_MODE) != 0U) ? 1U : 0U;
    }

    state->baseline_ok = ((state->has_fpu != 0U) &&
                          (state->has_msr != 0U) &&
                          (state->has_apic != 0U) &&
                          (state->has_sse != 0U) &&
                          (state->has_sse2 != 0U) &&
                          (state->has_fast_syscall != 0U) &&
                          (state->has_nx != 0U) &&
                          (state->has_long_mode != 0U)) ? 1U : 0U;

    report_syscall_abi_plan(state);
}
