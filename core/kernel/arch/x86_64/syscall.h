#ifndef LIAM_OS_X86_64_SYSCALL_H
#define LIAM_OS_X86_64_SYSCALL_H

#include "console.h"
#include "gdt.h"
#include "types.h"

#define X86_64_SYSCALL_MAX_ARGS 6U
#define X86_64_SYSCALL_SERVICE_EXIT 1U
#define X86_64_SYSCALL_SERVICE_WRITE 2U
#define X86_64_SYSCALL_SERVICE_OPEN 3U
#define X86_64_SYSCALL_SERVICE_READ 4U
#define X86_64_SYSCALL_SERVICE_CLOSE 5U
#define X86_64_SYSCALL_SERVICE_STAT 6U
#define X86_64_SYSCALL_SERVICE_GET_ARG 7U
#define X86_64_SYSCALL_SERVICE_EXEC 8U
#define X86_64_SYSCALL_SERVICE_GETPID 9U
#define X86_64_SYSCALL_SERVICE_YIELD 10U
#define X86_64_SYSCALL_SERVICE_PS 11U
#define X86_64_SYSCALL_SERVICE_WAIT 12U
#define X86_64_SYSCALL_SERVICE_DESKTOP_STATUS 13U
#define X86_64_SYSCALL_SERVICE_WINDOW_PRESENT 14U
#define X86_64_SYSCALL_SERVICE_TICKS 15U
#define X86_64_SYSCALL_SERVICE_SLEEP_TICKS 16U
#define X86_64_SYSCALL_SERVICE_INPUT_STATUS 17U
#define X86_64_SYSCALL_SERVICE_INPUT_EVENTS 18U
#define X86_64_SYSCALL_SERVICE_SPAWN 19U
#define X86_64_SYSCALL_SERVICE_MKDIR 20U
#define X86_64_SYSCALL_SERVICE_UNLINK 21U
#define X86_64_SYSCALL_SERVICE_COUNT 21U
#define X86_64_SYSCALL_SERVICE_COUNT_OK 1U
#define X86_64_MSR_IA32_EFER 0xC0000080ULL
#define X86_64_MSR_IA32_STAR 0xC0000081ULL
#define X86_64_MSR_IA32_LSTAR 0xC0000082ULL
#define X86_64_MSR_IA32_FMASK 0xC0000084ULL
#define X86_64_SYSCALL_FMASK_VALUE 0x0000000000000200ULL
#define X86_64_SYSCALL_STAR_VALUE \
    ((((u64)X86_64_GDT_SYSCALL_USER_SELECTOR_BASE) << 48U) | (((u64)X86_64_GDT_CODE_SELECTOR) << 32U))

extern void x86_64_syscall_entry_stub(void);

struct x86_64_syscall_abi_state {
    u32 initialized;
    u32 fast_syscall_supported;
    u32 syscall_sysret_planned;
    u32 msr_programming_deferred;
    u32 msr_programming_required;
    u32 entry_stub_installed;
    u32 user_entry_deferred;
    u32 user_selectors_ready;
    u32 arg_register_count;
    u32 syscall_number_rax;
    u32 return_rax;
    u32 arg0_rdi;
    u32 arg1_rsi;
    u32 arg2_rdx;
    u32 arg3_r10;
    u32 arg4_r8;
    u32 arg5_r9;
    u32 clobbers_rcx_r11;
    u32 user_pointer_validation_required;
    u32 service_count;
    u32 service_exit_planned;
    u32 service_write_planned;
    u32 service_open_planned;
    u32 service_read_planned;
    u32 service_close_planned;
    u32 service_stat_planned;
    u32 service_get_arg_planned;
    u32 service_exec_planned;
    u32 service_getpid_planned;
    u32 service_yield_planned;
    u32 service_ps_planned;
    u32 service_wait_planned;
    u32 service_desktop_status_planned;
    u32 service_window_present_planned;
    u32 service_ticks_planned;
    u32 service_sleep_ticks_planned;
    u32 service_input_status_planned;
    u32 service_input_events_planned;
    u32 service_spawn_planned;
    u32 service_mkdir_planned;
    u32 service_unlink_planned;
    u32 syscall_abi_ok;
    u32 syscall_entry_ready;
    u64 entry_lstar_target;
    u64 planned_star_value;
    u64 planned_fmask_value;
    u64 ia32_efer_msr;
    u64 ia32_star_msr;
    u64 ia32_lstar_msr;
    u64 ia32_fmask_msr;
};

static inline void x86_64_syscall_abi_init(struct x86_64_syscall_abi_state *state, u32 fast_syscall_supported)
{
    state->initialized = 1U;
    state->fast_syscall_supported = fast_syscall_supported;
    state->syscall_sysret_planned = 1U;
    state->msr_programming_deferred = 0U;
    state->msr_programming_required = 1U;
    state->entry_lstar_target = (u64)x86_64_syscall_entry_stub;
    state->entry_stub_installed = (state->entry_lstar_target != 0ULL) ? 1U : 0U;
    state->user_entry_deferred = 0U;
    state->user_selectors_ready = 1U;
    state->arg_register_count = X86_64_SYSCALL_MAX_ARGS;
    state->syscall_number_rax = 1U;
    state->return_rax = 1U;
    state->arg0_rdi = 1U;
    state->arg1_rsi = 1U;
    state->arg2_rdx = 1U;
    state->arg3_r10 = 1U;
    state->arg4_r8 = 1U;
    state->arg5_r9 = 1U;
    state->clobbers_rcx_r11 = 1U;
    state->user_pointer_validation_required = 1U;
    state->service_count = X86_64_SYSCALL_SERVICE_COUNT;
    state->service_exit_planned = (X86_64_SYSCALL_SERVICE_EXIT == 1U) ? 1U : 0U;
    state->service_write_planned = (X86_64_SYSCALL_SERVICE_WRITE == 2U) ? 1U : 0U;
    state->service_open_planned = (X86_64_SYSCALL_SERVICE_OPEN == 3U) ? 1U : 0U;
    state->service_read_planned = (X86_64_SYSCALL_SERVICE_READ == 4U) ? 1U : 0U;
    state->service_close_planned = (X86_64_SYSCALL_SERVICE_CLOSE == 5U) ? 1U : 0U;
    state->service_stat_planned = (X86_64_SYSCALL_SERVICE_STAT == 6U) ? 1U : 0U;
    state->service_get_arg_planned = (X86_64_SYSCALL_SERVICE_GET_ARG == 7U) ? 1U : 0U;
    state->service_exec_planned = (X86_64_SYSCALL_SERVICE_EXEC == 8U) ? 1U : 0U;
    state->service_getpid_planned = (X86_64_SYSCALL_SERVICE_GETPID == 9U) ? 1U : 0U;
    state->service_yield_planned = (X86_64_SYSCALL_SERVICE_YIELD == 10U) ? 1U : 0U;
    state->service_ps_planned = (X86_64_SYSCALL_SERVICE_PS == 11U) ? 1U : 0U;
    state->service_wait_planned = (X86_64_SYSCALL_SERVICE_WAIT == 12U) ? 1U : 0U;
    state->service_desktop_status_planned =
        (X86_64_SYSCALL_SERVICE_DESKTOP_STATUS == 13U) ? 1U : 0U;
    state->service_window_present_planned =
        (X86_64_SYSCALL_SERVICE_WINDOW_PRESENT == 14U) ? 1U : 0U;
    state->service_ticks_planned = (X86_64_SYSCALL_SERVICE_TICKS == 15U) ? 1U : 0U;
    state->service_sleep_ticks_planned =
        (X86_64_SYSCALL_SERVICE_SLEEP_TICKS == 16U) ? 1U : 0U;
    state->service_input_status_planned = (X86_64_SYSCALL_SERVICE_INPUT_STATUS == 17U) ? 1U : 0U;
    state->service_input_events_planned = (X86_64_SYSCALL_SERVICE_INPUT_EVENTS == 18U) ? 1U : 0U;
    state->service_spawn_planned = (X86_64_SYSCALL_SERVICE_SPAWN == 19U) ? 1U : 0U;
    state->service_mkdir_planned = (X86_64_SYSCALL_SERVICE_MKDIR == 20U) ? 1U : 0U;
    state->service_unlink_planned = (X86_64_SYSCALL_SERVICE_UNLINK == 21U) ? 1U : 0U;
    state->planned_star_value = X86_64_SYSCALL_STAR_VALUE;
    state->planned_fmask_value = X86_64_SYSCALL_FMASK_VALUE;
    state->ia32_efer_msr = X86_64_MSR_IA32_EFER;
    state->ia32_star_msr = X86_64_MSR_IA32_STAR;
    state->ia32_lstar_msr = X86_64_MSR_IA32_LSTAR;
    state->ia32_fmask_msr = X86_64_MSR_IA32_FMASK;

    state->syscall_abi_ok = ((state->initialized != 0U) &&
                             (state->fast_syscall_supported != 0U) &&
                             (state->syscall_sysret_planned != 0U) &&
                             (state->msr_programming_deferred == 0U) &&
                             (state->msr_programming_required != 0U) &&
                             (state->entry_stub_installed != 0U) &&
                             (state->user_entry_deferred == 0U) &&
                             (state->user_selectors_ready != 0U) &&
                             (state->planned_star_value != 0ULL) &&
                             (state->planned_fmask_value != 0ULL) &&
                             (state->arg_register_count == X86_64_SYSCALL_MAX_ARGS) &&
                             (state->syscall_number_rax != 0U) &&
                             (state->return_rax != 0U) &&
                             (state->arg0_rdi != 0U) &&
                             (state->arg1_rsi != 0U) &&
                             (state->arg2_rdx != 0U) &&
                             (state->arg3_r10 != 0U) &&
                             (state->arg4_r8 != 0U) &&
                             (state->arg5_r9 != 0U) &&
                             (state->clobbers_rcx_r11 != 0U) &&
                             (state->user_pointer_validation_required != 0U) &&
                             (state->service_count == X86_64_SYSCALL_SERVICE_COUNT) &&
                             (state->service_exit_planned != 0U) &&
                             (state->service_write_planned != 0U) &&
                             (state->service_open_planned != 0U) &&
                             (state->service_read_planned != 0U) &&
                             (state->service_close_planned != 0U) &&
                             (state->service_stat_planned != 0U) &&
                             (state->service_get_arg_planned != 0U) &&
                             (state->service_exec_planned != 0U) &&
                             (state->service_getpid_planned != 0U) &&
                             (state->service_yield_planned != 0U) &&
                             (state->service_ps_planned != 0U) &&
                             (state->service_wait_planned != 0U) &&
                             (state->service_desktop_status_planned != 0U) &&
                             (state->service_window_present_planned != 0U) &&
                             (state->service_ticks_planned != 0U) &&
                             (state->service_sleep_ticks_planned != 0U) &&
                             (state->service_input_status_planned != 0U) &&
                             (state->service_input_events_planned != 0U) &&
                             (state->service_spawn_planned != 0U) &&
                             (state->service_mkdir_planned != 0U) &&
                             (state->service_unlink_planned != 0U)) ? 1U : 0U;
    state->syscall_entry_ready = ((state->syscall_abi_ok != 0U) &&
                                  (state->entry_stub_installed != 0U)) ? 1U : 0U;
}

static inline void x86_64_syscall_abi_report(const struct x86_64_syscall_abi_state *state)
{
    x86_64_serial_write_line("x86_64 syscall ABI plan online");
    x86_64_serial_write_u32("Syscall fast supported: ", state->fast_syscall_supported);
    x86_64_serial_write_u32("Syscall/SYSRET planned: ", state->syscall_sysret_planned);
    x86_64_serial_write_u32("Syscall args: ", state->arg_register_count);
    x86_64_serial_write_u32("Syscall number RAX: ", state->syscall_number_rax);
    x86_64_serial_write_u32("Syscall return RAX: ", state->return_rax);
    x86_64_serial_write_u32("Syscall arg0 RDI: ", state->arg0_rdi);
    x86_64_serial_write_u32("Syscall arg1 RSI: ", state->arg1_rsi);
    x86_64_serial_write_u32("Syscall arg2 RDX: ", state->arg2_rdx);
    x86_64_serial_write_u32("Syscall arg3 R10: ", state->arg3_r10);
    x86_64_serial_write_u32("Syscall arg4 R8: ", state->arg4_r8);
    x86_64_serial_write_u32("Syscall arg5 R9: ", state->arg5_r9);
    x86_64_serial_write_u32("Syscall clobbers RCX/R11: ", state->clobbers_rcx_r11);
    x86_64_serial_write_u32("Syscall pointer validation required: ", state->user_pointer_validation_required);
    x86_64_serial_write_u32("Syscall service count: ", state->service_count);
    x86_64_serial_write_u32("Syscall exit planned: ", state->service_exit_planned);
    x86_64_serial_write_u32("Syscall write planned: ", state->service_write_planned);
    x86_64_serial_write_u32("Syscall open planned: ", state->service_open_planned);
    x86_64_serial_write_u32("Syscall read planned: ", state->service_read_planned);
    x86_64_serial_write_u32("Syscall close planned: ", state->service_close_planned);
    x86_64_serial_write_u32("Syscall stat planned: ", state->service_stat_planned);
    x86_64_serial_write_u32("Syscall get_arg planned: ", state->service_get_arg_planned);
    x86_64_serial_write_u32("Syscall exec planned: ", state->service_exec_planned);
    x86_64_serial_write_u32("Syscall getpid planned: ", state->service_getpid_planned);
    x86_64_serial_write_u32("Syscall yield planned: ", state->service_yield_planned);
    x86_64_serial_write_u32("Syscall ps planned: ", state->service_ps_planned);
    x86_64_serial_write_u32("Syscall wait planned: ", state->service_wait_planned);
    x86_64_serial_write_u32("Syscall desktop status planned: ", state->service_desktop_status_planned);
    x86_64_serial_write_u32("Syscall window present planned: ", state->service_window_present_planned);
    x86_64_serial_write_u32("Syscall ticks planned: ", state->service_ticks_planned);
    x86_64_serial_write_u32("Syscall sleep ticks planned: ", state->service_sleep_ticks_planned);
    x86_64_serial_write_u32("Syscall input status planned: ", state->service_input_status_planned);
    x86_64_serial_write_u32("Syscall input events planned: ", state->service_input_events_planned);
    x86_64_serial_write_u32("Syscall spawn planned: ", state->service_spawn_planned);
    x86_64_serial_write_u32("Syscall mkdir planned: ", state->service_mkdir_planned);
    x86_64_serial_write_u32("Syscall unlink planned: ", state->service_unlink_planned);
    x86_64_serial_write_u32("Syscall MSR programming deferred: ", state->msr_programming_deferred);
    x86_64_serial_write_u32("Syscall MSR programming required: ", state->msr_programming_required);
    x86_64_serial_write_u32("Syscall user entry deferred: ", state->user_entry_deferred);
    x86_64_serial_write_u32("Syscall STAR selectors ready: ", state->user_selectors_ready);
    x86_64_serial_write_u32("Syscall entry stub installed: ", state->entry_stub_installed);
    x86_64_serial_write_hex64("Syscall LSTAR target: 0x", state->entry_lstar_target);
    x86_64_serial_write_hex64("Syscall planned STAR: 0x", state->planned_star_value);
    x86_64_serial_write_hex64("Syscall planned FMASK: 0x", state->planned_fmask_value);
    x86_64_serial_write_hex64("Syscall IA32_EFER MSR: ", state->ia32_efer_msr);
    x86_64_serial_write_hex64("Syscall IA32_STAR MSR: ", state->ia32_star_msr);
    x86_64_serial_write_hex64("Syscall IA32_LSTAR MSR: ", state->ia32_lstar_msr);
    x86_64_serial_write_hex64("Syscall IA32_FMASK MSR: ", state->ia32_fmask_msr);
    x86_64_serial_write_u32("Syscall entry ready: ", state->syscall_entry_ready);
    x86_64_serial_write_u32("Syscall ABI ok: ", state->syscall_abi_ok);
}

#endif
