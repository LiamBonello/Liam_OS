#include "syscall_msr.h"

#include "console.h"
#include "syscall.h"

#define X86_64_EFER_SCE (1ULL << 0U)

static u64 read_msr(u64 msr)
{
    u32 low;
    u32 high;

    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"((u32)msr));
    return ((u64)high << 32U) | (u64)low;
}

static void write_msr(u64 msr, u64 value)
{
    __asm__ volatile ("wrmsr"
                      :
                      : "c"((u32)msr), "a"((u32)value), "d"((u32)(value >> 32U))
                      : "memory");
}

static void clear_state(struct x86_64_syscall_msr_state *state)
{
    u8 *bytes = (u8 *)state;
    for (usize i = 0; i < sizeof(*state); ++i) {
        bytes[i] = 0U;
    }
}

void x86_64_syscall_msr_program(struct x86_64_syscall_msr_state *state,
                                u32 fast_syscall_supported,
                                u32 msr_supported)
{
    clear_state(state);
    state->initialized = 1U;
    state->fast_syscall_supported = fast_syscall_supported;
    state->msr_supported = msr_supported;
    state->star_value = X86_64_SYSCALL_STAR_VALUE;
    state->lstar_value = (u64)x86_64_syscall_entry_stub;
    state->fmask_value = X86_64_SYSCALL_FMASK_VALUE;

    if (fast_syscall_supported == 0U || msr_supported == 0U || state->lstar_value == 0ULL) {
        return;
    }

    state->efer_before = read_msr(X86_64_MSR_IA32_EFER);
    write_msr(X86_64_MSR_IA32_EFER, state->efer_before | X86_64_EFER_SCE);
    write_msr(X86_64_MSR_IA32_STAR, state->star_value);
    write_msr(X86_64_MSR_IA32_LSTAR, state->lstar_value);
    write_msr(X86_64_MSR_IA32_FMASK, state->fmask_value);

    state->efer_after = read_msr(X86_64_MSR_IA32_EFER);
    state->star_programmed = (read_msr(X86_64_MSR_IA32_STAR) == state->star_value) ? 1U : 0U;
    state->lstar_programmed = (read_msr(X86_64_MSR_IA32_LSTAR) == state->lstar_value) ? 1U : 0U;
    state->fmask_programmed = (read_msr(X86_64_MSR_IA32_FMASK) == state->fmask_value) ? 1U : 0U;
    state->efer_sce_enabled = ((state->efer_after & X86_64_EFER_SCE) != 0ULL) ? 1U : 0U;
    state->msr_programmed =
        ((state->efer_sce_enabled != 0U) &&
         (state->star_programmed != 0U) &&
         (state->lstar_programmed != 0U) &&
         (state->fmask_programmed != 0U)) ? 1U : 0U;
    state->msr_ok = state->msr_programmed;
}

void x86_64_syscall_msr_report(const struct x86_64_syscall_msr_state *state)
{
    x86_64_serial_write_line("x86_64 syscall MSRs online");
    x86_64_serial_write_u32("Syscall MSR initialized: ", state->initialized);
    x86_64_serial_write_u32("Syscall MSR supported: ", state->msr_supported);
    x86_64_serial_write_u32("Syscall fast supported for MSR: ", state->fast_syscall_supported);
    x86_64_serial_write_hex64("Syscall EFER before: 0x", state->efer_before);
    x86_64_serial_write_hex64("Syscall EFER after: 0x", state->efer_after);
    x86_64_serial_write_u32("Syscall EFER SCE enabled: ", state->efer_sce_enabled);
    x86_64_serial_write_hex64("Syscall STAR programmed value: 0x", state->star_value);
    x86_64_serial_write_hex64("Syscall LSTAR programmed value: 0x", state->lstar_value);
    x86_64_serial_write_hex64("Syscall FMASK programmed value: 0x", state->fmask_value);
    x86_64_serial_write_u32("Syscall STAR programmed: ", state->star_programmed);
    x86_64_serial_write_u32("Syscall LSTAR programmed: ", state->lstar_programmed);
    x86_64_serial_write_u32("Syscall FMASK programmed: ", state->fmask_programmed);
    x86_64_serial_write_u32("Syscall MSRs programmed: ", state->msr_programmed);
    x86_64_serial_write_u32("Syscall MSR ok: ", state->msr_ok);
}
