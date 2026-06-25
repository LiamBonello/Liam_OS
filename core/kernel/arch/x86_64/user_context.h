#ifndef LIAM_OS_X86_64_USER_CONTEXT_H
#define LIAM_OS_X86_64_USER_CONTEXT_H

#include "console.h"
#include "gdt.h"
#include "process.h"
#include "types.h"
#include "userland.h"

#define X86_64_USER_RFLAGS_RESERVED_ONE 0x0000000000000002ULL
#define X86_64_USER_RFLAGS_INTERRUPT_ENABLE 0x0000000000000200ULL
#define X86_64_USER_RFLAGS_BASE \
    (X86_64_USER_RFLAGS_RESERVED_ONE | X86_64_USER_RFLAGS_INTERRUPT_ENABLE)
#define X86_64_USER_ENTRY_STACK_ALIGNMENT 16ULL
#define X86_64_USER_CONTEXT_ARG0 0x0000000000000000ULL

enum x86_64_user_context_state_code {
    X86_64_USER_CONTEXT_EMPTY = 0,
    X86_64_USER_CONTEXT_READY = 1,
    X86_64_USER_CONTEXT_RUNNING = 2,
    X86_64_USER_CONTEXT_EXITED = 3,
    X86_64_USER_CONTEXT_FAULTED = 4
};

struct x86_64_user_context_frame {
    u64 rip;
    u64 rsp;
    u64 rflags;
    u64 cs;
    u64 ss;
    u64 arg0;
};

struct x86_64_user_context_state {
    u32 initialized;
    u32 state;
    u32 entry_canonical;
    u32 stack_canonical;
    u32 entry_page_aligned;
    u32 stack_aligned;
    u32 selectors_ok;
    u32 rflags_ok;
    u32 cr3_page_aligned;
    u32 address_space_ready;
    u32 elf_entry_bound;
    u32 user_stack_mapped_planned;
    u32 user_code_mapped_planned;
    u32 transition_frame_ok;
    u32 cr3_probe_user_process_selected;
    u32 cr3_probe_user_pid;
    u32 cr3_probe_attempted;
    u32 cr3_probe_switched;
    u32 cr3_probe_restored;
    u32 cr3_probe_entry_read_ok;
    u32 cr3_probe_stack_read_ok;
    u32 cr3_probe_ok;
    u32 context_ok;
    u64 address_space_id;
    u64 cr3_planned;
    u64 cr3_probe_previous;
    u64 cr3_probe_active;
    u64 cr3_probe_restored_value;
    u32 cr3_probe_entry_value;
    u32 cr3_probe_stack_value;
    struct x86_64_user_context_frame frame;
};

static inline u64 x86_64_user_context_read_cr3(void)
{
    u64 value;
    __asm__ volatile ("movq %%cr3, %0" : "=r"(value));
    return value;
}

static inline void x86_64_user_context_write_cr3(u64 value)
{
    __asm__ volatile ("movq %0, %%cr3" :: "r"(value) : "memory");
}

static inline u32 x86_64_user_context_read_u8(u64 address)
{
    volatile const u8 *ptr = (volatile const u8 *)address;
    return (u32)(*ptr);
}

static inline void x86_64_user_context_probe_cr3(struct x86_64_user_context_state *state)
{
    state->cr3_probe_attempted = 1U;
    state->cr3_probe_previous = x86_64_user_context_read_cr3();

    x86_64_user_context_write_cr3(state->cr3_planned);
    state->cr3_probe_active = x86_64_user_context_read_cr3();
    state->cr3_probe_switched = (state->cr3_probe_active == state->cr3_planned) ? 1U : 0U;

    if (state->cr3_probe_switched != 0U) {
        state->cr3_probe_entry_value = x86_64_user_context_read_u8(state->frame.rip);
        state->cr3_probe_stack_value = x86_64_user_context_read_u8(state->frame.rsp);
        state->cr3_probe_entry_read_ok = 1U;
        state->cr3_probe_stack_read_ok = 1U;
    }

    x86_64_user_context_write_cr3(state->cr3_probe_previous);
    state->cr3_probe_restored_value = x86_64_user_context_read_cr3();
    state->cr3_probe_restored =
        (state->cr3_probe_restored_value == state->cr3_probe_previous) ? 1U : 0U;
    state->cr3_probe_ok =
        ((state->cr3_probe_switched != 0U) &&
         (state->cr3_probe_entry_read_ok != 0U) &&
         (state->cr3_probe_stack_read_ok != 0U) &&
         (state->cr3_probe_restored != 0U)) ? 1U : 0U;
}

static inline void x86_64_user_context_init(struct x86_64_user_context_state *state,
                                            const struct x86_64_userland_foundation_state *foundation,
                                            u64 address_space_id,
                                            u64 planned_cr3)
{
    const struct x86_64_process_smoke_state *process_smoke = x86_64_process_get_smoke_state();
    u64 effective_cr3 = planned_cr3;

    if (process_smoke != (const struct x86_64_process_smoke_state *)0 &&
        process_smoke->user_scheduler_ready != 0U &&
        process_smoke->last_user_cr3 != 0ULL) {
        effective_cr3 = process_smoke->last_user_cr3;
        state->cr3_probe_user_process_selected = 1U;
        state->cr3_probe_user_pid = process_smoke->last_scheduled_user_pid;
    }

    state->initialized = 1U;
    state->state = X86_64_USER_CONTEXT_READY;
    state->address_space_id = address_space_id;
    state->cr3_planned = effective_cr3;
    state->frame.rip = foundation->sample_elf_entry;
    state->frame.rsp = foundation->user_stack_top - X86_64_USER_ENTRY_STACK_ALIGNMENT;
    state->frame.rflags = X86_64_USER_RFLAGS_BASE;
    state->frame.cs = X86_64_GDT_USER_CODE_SELECTOR;
    state->frame.ss = X86_64_GDT_USER_DATA_SELECTOR;
    state->frame.arg0 = X86_64_USER_CONTEXT_ARG0;

    state->entry_canonical = x86_64_user_is_low_canonical(state->frame.rip);
    state->stack_canonical = x86_64_user_is_low_canonical(state->frame.rsp);
    state->entry_page_aligned = x86_64_user_is_page_aligned(state->frame.rip);
    state->stack_aligned = ((state->frame.rsp & (X86_64_USER_ENTRY_STACK_ALIGNMENT - 1ULL)) == 0ULL) ? 1U : 0U;
    state->selectors_ok =
        ((state->frame.cs == X86_64_GDT_USER_CODE_SELECTOR) &&
         (state->frame.ss == X86_64_GDT_USER_DATA_SELECTOR) &&
         ((state->frame.cs & 3ULL) == 3ULL) &&
         ((state->frame.ss & 3ULL) == 3ULL)) ? 1U : 0U;
    state->rflags_ok =
        (((state->frame.rflags & X86_64_USER_RFLAGS_RESERVED_ONE) != 0ULL) &&
         ((state->frame.rflags & X86_64_USER_RFLAGS_INTERRUPT_ENABLE) != 0ULL)) ? 1U : 0U;
    state->cr3_page_aligned = x86_64_user_is_page_aligned(state->cr3_planned);
    state->address_space_ready =
        ((state->address_space_id != 0ULL) &&
         (state->cr3_planned != 0ULL) &&
         (state->cr3_page_aligned != 0U)) ? 1U : 0U;
    state->elf_entry_bound =
        ((foundation->foundation_ok != 0U) &&
         (state->frame.rip == foundation->sample_elf_entry) &&
         (state->frame.rip == X86_64_USER_CODE_BASE)) ? 1U : 0U;
    state->user_stack_mapped_planned =
        ((foundation->user_stack_base < state->frame.rsp) &&
         (state->frame.rsp < foundation->user_stack_top)) ? 1U : 0U;
    state->user_code_mapped_planned =
        ((foundation->user_code_base == X86_64_USER_CODE_BASE) &&
         (foundation->elf64_entry_ok != 0U)) ? 1U : 0U;
    state->transition_frame_ok =
        ((state->entry_canonical != 0U) &&
         (state->stack_canonical != 0U) &&
         (state->entry_page_aligned != 0U) &&
         (state->stack_aligned != 0U) &&
         (state->selectors_ok != 0U) &&
         (state->rflags_ok != 0U)) ? 1U : 0U;

    if ((state->address_space_ready != 0U) &&
        (state->transition_frame_ok != 0U) &&
        (state->user_code_mapped_planned != 0U) &&
        (state->user_stack_mapped_planned != 0U)) {
        x86_64_user_context_probe_cr3(state);
    }

    state->context_ok =
        ((state->initialized != 0U) &&
         (state->state == X86_64_USER_CONTEXT_READY) &&
         (state->address_space_ready != 0U) &&
         (state->elf_entry_bound != 0U) &&
         (state->user_stack_mapped_planned != 0U) &&
         (state->user_code_mapped_planned != 0U) &&
         (state->transition_frame_ok != 0U) &&
         (state->cr3_probe_ok != 0U)) ? 1U : 0U;
}

static inline void x86_64_user_context_report(const struct x86_64_user_context_state *state)
{
    x86_64_serial_write_line("x86_64 user execution context online");
    x86_64_serial_write_u32("User context initialized: ", state->initialized);
    x86_64_serial_write_u32("User context state: ", state->state);
    x86_64_serial_write_hex64("User address space id: 0x", state->address_space_id);
    x86_64_serial_write_hex64("User planned CR3: 0x", state->cr3_planned);
    x86_64_serial_write_u32("User CR3 page aligned: ", state->cr3_page_aligned);
    x86_64_serial_write_u32("User address space ready: ", state->address_space_ready);
    x86_64_serial_write_hex64("User RIP: 0x", state->frame.rip);
    x86_64_serial_write_hex64("User RSP: 0x", state->frame.rsp);
    x86_64_serial_write_hex64("User RFLAGS: 0x", state->frame.rflags);
    x86_64_serial_write_hex64("User CS: 0x", state->frame.cs);
    x86_64_serial_write_hex64("User SS: 0x", state->frame.ss);
    x86_64_serial_write_hex64("User ARG0: 0x", state->frame.arg0);
    x86_64_serial_write_u32("User entry canonical: ", state->entry_canonical);
    x86_64_serial_write_u32("User stack canonical: ", state->stack_canonical);
    x86_64_serial_write_u32("User entry aligned: ", state->entry_page_aligned);
    x86_64_serial_write_u32("User stack aligned: ", state->stack_aligned);
    x86_64_serial_write_u32("User selectors ok: ", state->selectors_ok);
    x86_64_serial_write_u32("User RFLAGS ok: ", state->rflags_ok);
    x86_64_serial_write_u32("User ELF entry bound: ", state->elf_entry_bound);
    x86_64_serial_write_u32("User code mapping planned: ", state->user_code_mapped_planned);
    x86_64_serial_write_u32("User stack mapping planned: ", state->user_stack_mapped_planned);
    x86_64_serial_write_u32("User transition frame ok: ", state->transition_frame_ok);
    x86_64_serial_write_u32("User CR3 probe user process selected: ", state->cr3_probe_user_process_selected);
    x86_64_serial_write_u32("User CR3 probe user pid: ", state->cr3_probe_user_pid);
    x86_64_serial_write_u32("User CR3 probe attempted: ", state->cr3_probe_attempted);
    x86_64_serial_write_hex64("User CR3 probe previous: 0x", state->cr3_probe_previous);
    x86_64_serial_write_hex64("User CR3 probe active: 0x", state->cr3_probe_active);
    x86_64_serial_write_hex64("User CR3 probe restored: 0x", state->cr3_probe_restored_value);
    x86_64_serial_write_u32("User CR3 probe switched: ", state->cr3_probe_switched);
    x86_64_serial_write_u32("User CR3 probe restored ok: ", state->cr3_probe_restored);
    x86_64_serial_write_hex32("User CR3 probe entry byte: 0x", state->cr3_probe_entry_value);
    x86_64_serial_write_hex32("User CR3 probe stack byte: 0x", state->cr3_probe_stack_value);
    x86_64_serial_write_u32("User CR3 probe entry read ok: ", state->cr3_probe_entry_read_ok);
    x86_64_serial_write_u32("User CR3 probe stack read ok: ", state->cr3_probe_stack_read_ok);
    x86_64_serial_write_u32("User CR3 probe ok: ", state->cr3_probe_ok);
    x86_64_serial_write_u32("User context ok: ", state->context_ok);
}

#endif
