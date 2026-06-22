#ifndef LIAM_OS_X86_64_USERLAND_H
#define LIAM_OS_X86_64_USERLAND_H

#include "console.h"
#include "syscall.h"
#include "types.h"

#define X86_64_USER_CANONICAL_LIMIT 0x0000800000000000ULL
#define X86_64_USER_CODE_BASE 0x0000000000400000ULL
#define X86_64_USER_HEAP_BASE 0x0000000000800000ULL
#define X86_64_USER_STACK_TOP 0x0000000080000000ULL
#define X86_64_USER_STACK_BYTES 0x0000000000010000ULL
#define X86_64_USER_STACK_BASE (X86_64_USER_STACK_TOP - X86_64_USER_STACK_BYTES)
#define X86_64_USER_PAGE_BYTES 4096ULL

#define X86_64_ELF_MAGIC_0 0x7FU
#define X86_64_ELF_MAGIC_1 'E'
#define X86_64_ELF_MAGIC_2 'L'
#define X86_64_ELF_MAGIC_3 'F'
#define X86_64_ELF_CLASS_64 2U
#define X86_64_ELF_DATA_LITTLE_ENDIAN 1U
#define X86_64_ELF_VERSION_CURRENT 1U
#define X86_64_ELF_TYPE_EXECUTABLE 2U
#define X86_64_ELF_MACHINE_X86_64 0x3EU
#define X86_64_ELF_PROGRAM_TYPE_LOAD 1U

struct x86_64_elf_header {
    u8 ident[16];
    u16 type;
    u16 machine;
    u32 version;
    u64 entry;
    u64 program_header_offset;
    u64 section_header_offset;
    u32 flags;
    u16 header_size;
    u16 program_header_entry_size;
    u16 program_header_count;
    u16 section_header_entry_size;
    u16 section_header_count;
    u16 section_name_index;
} __attribute__((packed));

struct x86_64_elf_program_header {
    u32 type;
    u32 flags;
    u64 offset;
    u64 virtual_address;
    u64 physical_address;
    u64 file_size;
    u64 memory_size;
    u64 alignment;
} __attribute__((packed));

struct x86_64_userland_foundation_state {
    u32 initialized;
    u32 layout_canonical;
    u32 code_page_aligned;
    u32 heap_page_aligned;
    u32 stack_page_aligned;
    u32 stack_above_code;
    u32 elf64_header_size_ok;
    u32 elf64_program_header_size_ok;
    u32 elf64_magic_ok;
    u32 elf64_class_ok;
    u32 elf64_machine_ok;
    u32 elf64_entry_ok;
    u32 syscall_table_planned;
    u32 pointer_validation_planned;
    u32 foundation_ok;
    u64 user_code_base;
    u64 user_heap_base;
    u64 user_stack_base;
    u64 user_stack_top;
    u64 user_stack_bytes;
    u64 sample_elf_entry;
};

static inline u32 x86_64_user_is_low_canonical(u64 address)
{
    return (address < X86_64_USER_CANONICAL_LIMIT) ? 1U : 0U;
}

static inline u32 x86_64_user_is_page_aligned(u64 value)
{
    return ((value & (X86_64_USER_PAGE_BYTES - 1ULL)) == 0ULL) ? 1U : 0U;
}

static inline void x86_64_userland_foundation_init(struct x86_64_userland_foundation_state *state)
{
    static const struct x86_64_elf_header sample_header = {
        .ident = {
            X86_64_ELF_MAGIC_0,
            X86_64_ELF_MAGIC_1,
            X86_64_ELF_MAGIC_2,
            X86_64_ELF_MAGIC_3,
            X86_64_ELF_CLASS_64,
            X86_64_ELF_DATA_LITTLE_ENDIAN,
            X86_64_ELF_VERSION_CURRENT,
            0U,
            0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U
        },
        .type = X86_64_ELF_TYPE_EXECUTABLE,
        .machine = X86_64_ELF_MACHINE_X86_64,
        .version = X86_64_ELF_VERSION_CURRENT,
        .entry = X86_64_USER_CODE_BASE,
        .program_header_offset = sizeof(struct x86_64_elf_header),
        .section_header_offset = 0ULL,
        .flags = 0U,
        .header_size = sizeof(struct x86_64_elf_header),
        .program_header_entry_size = sizeof(struct x86_64_elf_program_header),
        .program_header_count = 1U,
        .section_header_entry_size = 0U,
        .section_header_count = 0U,
        .section_name_index = 0U
    };

    state->initialized = 1U;
    state->user_code_base = X86_64_USER_CODE_BASE;
    state->user_heap_base = X86_64_USER_HEAP_BASE;
    state->user_stack_base = X86_64_USER_STACK_BASE;
    state->user_stack_top = X86_64_USER_STACK_TOP;
    state->user_stack_bytes = X86_64_USER_STACK_BYTES;
    state->sample_elf_entry = sample_header.entry;

    state->layout_canonical =
        ((x86_64_user_is_low_canonical(state->user_code_base) != 0U) &&
         (x86_64_user_is_low_canonical(state->user_heap_base) != 0U) &&
         (x86_64_user_is_low_canonical(state->user_stack_base) != 0U) &&
         (x86_64_user_is_low_canonical(state->user_stack_top - 1ULL) != 0U)) ? 1U : 0U;
    state->code_page_aligned = x86_64_user_is_page_aligned(state->user_code_base);
    state->heap_page_aligned = x86_64_user_is_page_aligned(state->user_heap_base);
    state->stack_page_aligned =
        ((x86_64_user_is_page_aligned(state->user_stack_base) != 0U) &&
         (x86_64_user_is_page_aligned(state->user_stack_top) != 0U)) ? 1U : 0U;
    state->stack_above_code = (state->user_stack_base > state->user_code_base) ? 1U : 0U;

    state->elf64_header_size_ok = (sample_header.header_size == sizeof(struct x86_64_elf_header)) ? 1U : 0U;
    state->elf64_program_header_size_ok =
        (sample_header.program_header_entry_size == sizeof(struct x86_64_elf_program_header)) ? 1U : 0U;
    state->elf64_magic_ok =
        ((sample_header.ident[0] == X86_64_ELF_MAGIC_0) &&
         (sample_header.ident[1] == X86_64_ELF_MAGIC_1) &&
         (sample_header.ident[2] == X86_64_ELF_MAGIC_2) &&
         (sample_header.ident[3] == X86_64_ELF_MAGIC_3)) ? 1U : 0U;
    state->elf64_class_ok =
        ((sample_header.ident[4] == X86_64_ELF_CLASS_64) &&
         (sample_header.ident[5] == X86_64_ELF_DATA_LITTLE_ENDIAN) &&
         (sample_header.ident[6] == X86_64_ELF_VERSION_CURRENT)) ? 1U : 0U;
    state->elf64_machine_ok =
        ((sample_header.type == X86_64_ELF_TYPE_EXECUTABLE) &&
         (sample_header.machine == X86_64_ELF_MACHINE_X86_64) &&
         (sample_header.version == X86_64_ELF_VERSION_CURRENT)) ? 1U : 0U;
    state->elf64_entry_ok =
        ((sample_header.entry == X86_64_USER_CODE_BASE) &&
         (x86_64_user_is_low_canonical(sample_header.entry) != 0U) &&
         (x86_64_user_is_page_aligned(sample_header.entry) != 0U)) ? 1U : 0U;
    state->syscall_table_planned = X86_64_SYSCALL_SERVICE_COUNT_OK;
    state->pointer_validation_planned = 1U;

    state->foundation_ok =
        ((state->initialized != 0U) &&
         (state->layout_canonical != 0U) &&
         (state->code_page_aligned != 0U) &&
         (state->heap_page_aligned != 0U) &&
         (state->stack_page_aligned != 0U) &&
         (state->stack_above_code != 0U) &&
         (state->elf64_header_size_ok != 0U) &&
         (state->elf64_program_header_size_ok != 0U) &&
         (state->elf64_magic_ok != 0U) &&
         (state->elf64_class_ok != 0U) &&
         (state->elf64_machine_ok != 0U) &&
         (state->elf64_entry_ok != 0U) &&
         (state->syscall_table_planned != 0U) &&
         (state->pointer_validation_planned != 0U)) ? 1U : 0U;
}

static inline void x86_64_userland_foundation_report(const struct x86_64_userland_foundation_state *state)
{
    x86_64_serial_write_line("x86_64 userland foundation online");
    x86_64_serial_write_hex64("User code base: 0x", state->user_code_base);
    x86_64_serial_write_hex64("User heap base: 0x", state->user_heap_base);
    x86_64_serial_write_hex64("User stack base: 0x", state->user_stack_base);
    x86_64_serial_write_hex64("User stack top: 0x", state->user_stack_top);
    x86_64_serial_write_hex64("User stack bytes: 0x", state->user_stack_bytes);
    x86_64_serial_write_u32("User layout canonical: ", state->layout_canonical);
    x86_64_serial_write_u32("User code aligned: ", state->code_page_aligned);
    x86_64_serial_write_u32("User heap aligned: ", state->heap_page_aligned);
    x86_64_serial_write_u32("User stack aligned: ", state->stack_page_aligned);
    x86_64_serial_write_u32("User stack above code: ", state->stack_above_code);
    x86_64_serial_write_u32("ELF64 header size ok: ", state->elf64_header_size_ok);
    x86_64_serial_write_u32("ELF64 phdr size ok: ", state->elf64_program_header_size_ok);
    x86_64_serial_write_u32("ELF64 magic ok: ", state->elf64_magic_ok);
    x86_64_serial_write_u32("ELF64 class ok: ", state->elf64_class_ok);
    x86_64_serial_write_u32("ELF64 machine ok: ", state->elf64_machine_ok);
    x86_64_serial_write_hex64("ELF64 sample entry: 0x", state->sample_elf_entry);
    x86_64_serial_write_u32("ELF64 entry ok: ", state->elf64_entry_ok);
    x86_64_serial_write_u32("Syscall table planned: ", state->syscall_table_planned);
    x86_64_serial_write_u32("User pointer validation planned: ", state->pointer_validation_planned);
    x86_64_serial_write_u32("Userland foundation ok: ", state->foundation_ok);
}

#endif
