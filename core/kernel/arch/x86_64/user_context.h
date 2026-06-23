#ifndef LIAM_OS_X86_64_USER_CONTEXT_H
#define LIAM_OS_X86_64_USER_CONTEXT_H

#include "console.h"
#include "gdt.h"
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
    u32 context_ok;
    u64 address_space_id;
    u64 cr3_planned;
    struct x86_64_user_context_frame frame;
};

static inline void x86_64_user_context_init(struct x86_64_user_context_state *state,
                                            const struct x86_64_userland_foundation_state *foundation,
                                            u64 address_space_id,
                                            u64 planned_cr3)
{
    state->initialized = 1U;
    state->state = X86_64_USER_CONTEXT_READY;
    state->address_space_id = address_space_id;
    state->cr3_planned = planned_cr3;
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
    state->context_ok =
        ((state->initialized != 0U) &&
         (state->state == X86_64_USER_CONTEXT_READY) &&
         (state->address_space_ready != 0U) &&
         (state->elf_entry_bound != 0U) &&
         (state->user_stack_mapped_planned != 0U) &&
         (state->user_code_mapped_planned != 0U) &&
         (state->transition_frame_ok != 0U)) ? 1U : 0U;
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
    x86_64_serial_write_u32("User context ok: ", state->context_ok);
}

#endif
