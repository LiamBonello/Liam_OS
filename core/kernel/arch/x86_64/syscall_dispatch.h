#ifndef LIAM_OS_X86_64_SYSCALL_DISPATCH_H
#define LIAM_OS_X86_64_SYSCALL_DISPATCH_H

#include "boot_info.h"
#include "console.h"
#include "desktop_services.h"
#include "idt.h"
#include "process.h"
#include "syscall.h"
#include "types.h"
#include "userland.h"
#include "vfs.h"

#define X86_64_SYSCALL_RET_OK 0ULL
#define X86_64_SYSCALL_RET_EFAULT 0xFFFFFFFFFFFFFFF2ULL
#define X86_64_SYSCALL_RET_EINVAL 0xFFFFFFFFFFFFFFEAULL
#define X86_64_SYSCALL_RET_ENOENT 0xFFFFFFFFFFFFFFFEULL
#define X86_64_SYSCALL_RET_ENOSYS 0xFFFFFFFFFFFFFFDAULL
#define X86_64_SYSCALL_STDIN 0ULL
#define X86_64_SYSCALL_STDOUT 1ULL
#define X86_64_SYSCALL_STDERR 2ULL
#define X86_64_SYSCALL_WRITE_MAX_BYTES 1024ULL
#define X86_64_SYSCALL_ARG_SHELL_MODE 0ULL
#define X86_64_ELF_PF_X 1U
#define X86_64_ELF_PF_W 2U

struct x86_64_syscall_frame {
    u64 number;
    u64 arg0;
    u64 arg1;
    u64 arg2;
    u64 arg3;
    u64 arg4;
    u64 arg5;
    u64 user_rip;
    u64 user_rflags;
    u64 result;
    u32 exit_requested;
    u32 cr3_switch_requested;
    u64 target_cr3;
};

struct x86_64_syscall_dispatch_state {
    u32 initialized;
    u32 service_count;
    u32 current_pid;
    u32 dispatch_calls;
    u32 pointer_validation_ok;
    u32 user_buffer_accepted;
    u32 kernel_pointer_rejected;
    u32 file_service_ready;
    u32 file_count;
    u32 open_file_count;
    u32 exit_ok;
    u32 write_ok;
    u32 write_fault_ok;
    u32 write_output_enabled;
    u32 read_ok;
    u32 read_fault_ok;
    u32 blocking_read_enabled;
    u32 blocking_read_spins;
    u32 get_arg_ok;
    u32 getpid_ok;
    u32 yield_ok;
    u32 ps_ok;
    u32 ps_fault_ok;
    u32 wait_ok;
    u32 wait_empty_ok;
    u32 wait_fault_ok;
    u32 desktop_status_ok;
    u32 desktop_status_fault_ok;
    u32 window_present_ok;
    u32 ticks_ok;
    u32 sleep_ticks_ok;
    u32 input_status_ok;
    u32 input_status_fault_ok;
    u32 input_events_ok;
    u32 input_events_empty_ok;
    u32 input_events_fault_ok;
    u32 input_event_cursor;
    u32 exec_fault_ok;
    u32 exec_path_found;
    u32 exec_type_ok;
    u32 exec_loaded;
    u32 exec_process_created;
    u32 exec_cr3_switch_requested;
    u32 exec_spawned_pid;
    u32 spawn_fault_ok;
    u32 spawn_path_found;
    u32 spawn_type_ok;
    u32 spawn_loaded;
    u32 spawn_process_created;
    u32 spawn_spawned_pid;
    u32 unknown_rejected;
    u32 dispatcher_ok;
    u32 exit_code;
    u32 yield_count;
    struct x86_64_vfs_state vfs;
    u64 exec_user_code_page;
    u64 exec_user_image_pages[X86_64_USER_IMAGE_MAX_PAGES];
    u32 exec_user_image_page_count;
    u32 exec_user_writable_page_bitmap;
    u64 exec_user_image_bytes;
    u64 exec_entry;
    u64 exec_target_cr3;
    u64 last_syscall;
    u64 last_result;
    u64 sample_user_buffer;
    u64 sample_kernel_pointer;
};

static inline u32 x86_64_syscall_user_range_ok(u64 address, u64 size)
{
    if (size == 0ULL) {
        return 1U;
    }

    if (address < X86_64_USER_CODE_BASE) {
        return 0U;
    }

    u64 end = address + size;
    if (end < address) {
        return 0U;
    }

    if (end > X86_64_USER_CANONICAL_LIMIT) {
        return 0U;
    }

    return 1U;
}

static inline u32 x86_64_syscall_user_cstring_valid(u64 user_address, u64 max_bytes)
{
    for (u64 i = 0ULL; i < max_bytes; ++i) {
        if (x86_64_syscall_user_range_ok(user_address + i, 1ULL) == 0U) {
            return 0U;
        }
        if (((const char *)user_address)[i] == '\0') {
            return 1U;
        }
    }

    return 0U;
}

static inline void x86_64_syscall_write_serial(const char *buffer, u64 size)
{
    u64 limit = size;
    if (limit > X86_64_SYSCALL_WRITE_MAX_BYTES) {
        limit = X86_64_SYSCALL_WRITE_MAX_BYTES;
    }

    for (u64 i = 0ULL; i < limit; ++i) {
        char message[2];
        message[0] = buffer[i];
        message[1] = '\0';
        x86_64_serial_write(message);
    }
}

static inline u64 x86_64_syscall_read_stdin(char *buffer, u64 size)
{
    u64 bytes = x86_64_keyboard_read(buffer, size);
    if (bytes < size) {
        bytes += x86_64_serial_read(buffer + bytes, size - bytes);
    }

    return bytes;
}

static inline u64 x86_64_syscall_read_stdin_blocking(struct x86_64_syscall_dispatch_state *state,
                                                     char *buffer,
                                                     u64 size)
{
    if (size == 0ULL) {
        return 0ULL;
    }

    u64 bytes = x86_64_syscall_read_stdin(buffer, size);
    while (bytes == 0ULL) {
        state->blocking_read_spins += 1U;
        __asm__ volatile("pause");
        bytes = x86_64_syscall_read_stdin(buffer, size);
    }

    return bytes;
}

static inline u64 x86_64_syscall_copy_string_arg(char *buffer, u64 size, const char *value)
{
    if (buffer == (char *)0 || size == 0ULL || value == (const char *)0) {
        return 0ULL;
    }

    u64 written = 0ULL;
    while (written + 1ULL < size && value[written] != '\0') {
        buffer[written] = value[written];
        written += 1ULL;
    }

    buffer[written] = '\0';
    return written;
}

static inline u64 x86_64_syscall_append_char(char *buffer, u64 size, u64 offset, char value)
{
    if (offset + 1ULL < size) {
        buffer[offset] = value;
    }

    return offset + 1ULL;
}

static inline u64 x86_64_syscall_append_string(char *buffer, u64 size, u64 offset, const char *value)
{
    if (value == (const char *)0) {
        return offset;
    }

    for (u64 i = 0ULL; value[i] != '\0'; ++i) {
        offset = x86_64_syscall_append_char(buffer, size, offset, value[i]);
    }

    return offset;
}

static inline u64 x86_64_syscall_append_u32(char *buffer, u64 size, u64 offset, u32 value)
{
    char digits[10];
    u32 count = 0U;

    if (value == 0U) {
        return x86_64_syscall_append_char(buffer, size, offset, '0');
    }

    while (value != 0U && count < (u32)sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (count > 0U) {
        --count;
        offset = x86_64_syscall_append_char(buffer, size, offset, digits[count]);
    }

    return offset;
}

static inline u64 x86_64_syscall_format_input_status(char *buffer, u64 size)
{
    struct x86_64_keyboard_state keyboard;
    x86_64_keyboard_get_state(&keyboard);

    u64 offset = 0ULL;
    offset = x86_64_syscall_append_string(buffer, size, offset, "input status\n");
    offset = x86_64_syscall_append_string(buffer, size, offset, "keyboard-ready ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.keyboard_ok);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');
    offset = x86_64_syscall_append_string(buffer, size, offset, "buffered ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.buffered_chars);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');
    offset = x86_64_syscall_append_string(buffer, size, offset, "capacity ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.buffer_capacity);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');
    offset = x86_64_syscall_append_string(buffer, size, offset, "scancodes ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.scancodes_seen);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');
    offset = x86_64_syscall_append_string(buffer, size, offset, "translated ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.translated_chars_seen);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');
    offset = x86_64_syscall_append_string(buffer, size, offset, "event-capacity ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.event_queue_capacity);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');
    offset = x86_64_syscall_append_string(buffer, size, offset, "event-queued ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.event_queued);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');
    offset = x86_64_syscall_append_string(buffer, size, offset, "event-drops ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.event_drops);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');
    offset = x86_64_syscall_append_string(buffer, size, offset, "event-oldest ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.event_oldest_sequence);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');
    offset = x86_64_syscall_append_string(buffer, size, offset, "event-latest ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.event_latest_sequence);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');
    offset = x86_64_syscall_append_string(buffer, size, offset, "events-read ");
    offset = x86_64_syscall_append_u32(buffer, size, offset, keyboard.events_read);
    offset = x86_64_syscall_append_char(buffer, size, offset, '\n');

    if (size != 0ULL) {
        if (offset < size) {
            buffer[offset] = '\0';
        } else {
            buffer[size - 1ULL] = '\0';
        }
    }

    return offset;
}

static inline u32 x86_64_syscall_image_range_fits(u64 offset, u64 size, u64 image_size)
{
    return ((offset <= image_size) && (size <= (image_size - offset))) ? 1U : 0U;
}

static inline const struct x86_64_elf_program_header *x86_64_syscall_program_header_at(
    const u8 *image,
    const struct x86_64_elf_header *header,
    u16 index)
{
    u64 offset = header->program_header_offset +
                 ((u64)index * (u64)header->program_header_entry_size);
    return (const struct x86_64_elf_program_header *)(image + offset);
}

static inline void x86_64_syscall_clear_user_image_pages(struct x86_64_syscall_dispatch_state *state)
{
    for (u32 page_index = 0U; page_index < X86_64_USER_IMAGE_MAX_PAGES; ++page_index) {
        u8 *page = (u8 *)state->exec_user_image_pages[page_index];
        if (page == (u8 *)0) {
            continue;
        }

        for (u64 i = 0ULL; i < X86_64_USER_PAGE_BYTES; ++i) {
            page[i] = 0U;
        }
    }

    state->exec_user_image_page_count = 0U;
    state->exec_user_writable_page_bitmap = 0U;
    state->exec_user_image_bytes = 0ULL;
}

static inline u32 x86_64_syscall_copy_to_user_image_page(struct x86_64_syscall_dispatch_state *state,
                                                         u64 image_offset,
                                                         u8 value)
{
    u32 page_index = (u32)(image_offset / X86_64_USER_PAGE_BYTES);
    if (page_index >= X86_64_USER_IMAGE_MAX_PAGES ||
        state->exec_user_image_pages[page_index] == 0ULL) {
        return 0U;
    }

    ((u8 *)state->exec_user_image_pages[page_index])[image_offset & (X86_64_USER_PAGE_BYTES - 1ULL)] = value;
    return 1U;
}

static inline u32 x86_64_syscall_load_exec_elf(struct x86_64_syscall_dispatch_state *state,
                                               const u8 *image,
                                               u64 image_size)
{
    if (state->exec_user_image_pages[0] == 0ULL ||
        image == (const u8 *)0 ||
        image_size < sizeof(struct x86_64_elf_header) ||
        image_size > X86_64_USER_STACK_BYTES) {
        return 0U;
    }

    const struct x86_64_elf_header *header = (const struct x86_64_elf_header *)image;
    u64 phdr_bytes = (u64)header->program_header_entry_size *
                     (u64)header->program_header_count;

    u32 header_ok =
        ((header->ident[0] == X86_64_ELF_MAGIC_0) &&
         (header->ident[1] == X86_64_ELF_MAGIC_1) &&
         (header->ident[2] == X86_64_ELF_MAGIC_2) &&
         (header->ident[3] == X86_64_ELF_MAGIC_3) &&
         (header->ident[4] == X86_64_ELF_CLASS_64) &&
         (header->ident[5] == X86_64_ELF_DATA_LITTLE_ENDIAN) &&
         (header->ident[6] == X86_64_ELF_VERSION_CURRENT) &&
         (header->type == X86_64_ELF_TYPE_EXECUTABLE) &&
         (header->machine == X86_64_ELF_MACHINE_X86_64) &&
         (header->version == X86_64_ELF_VERSION_CURRENT) &&
         (header->header_size == sizeof(struct x86_64_elf_header)) &&
         (header->program_header_entry_size == sizeof(struct x86_64_elf_program_header)) &&
         (header->program_header_count > 0U) &&
         (header->program_header_count <= 8U) &&
         (x86_64_syscall_image_range_fits(header->program_header_offset, phdr_bytes, image_size) != 0U)) ? 1U : 0U;
    if (header_ok == 0U) {
        return 0U;
    }

    x86_64_syscall_clear_user_image_pages(state);

    u32 load_seen = 0U;
    u32 entry_segment_seen = 0U;
    u64 image_bytes = 0ULL;
    for (u16 i = 0U; i < header->program_header_count; ++i) {
        const struct x86_64_elf_program_header *candidate =
            x86_64_syscall_program_header_at(image, header, i);
        if (candidate->type != X86_64_ELF_PROGRAM_TYPE_LOAD) {
            continue;
        }
        if (candidate->memory_size == 0ULL) {
            continue;
        }

        u64 segment_end = candidate->virtual_address + candidate->memory_size;
        if (segment_end < candidate->virtual_address ||
            candidate->virtual_address < X86_64_USER_CODE_BASE ||
            segment_end > (X86_64_USER_CODE_BASE + X86_64_USER_IMAGE_MAX_BYTES) ||
            candidate->alignment != X86_64_USER_PAGE_BYTES ||
            candidate->file_size > candidate->memory_size ||
            x86_64_syscall_image_range_fits(candidate->offset, candidate->file_size, image_size) == 0U) {
            return 0U;
        }

        u64 segment_offset = candidate->virtual_address - X86_64_USER_CODE_BASE;
        u64 segment_image_end = segment_offset + candidate->memory_size;
        u32 first_page = (u32)(segment_offset / X86_64_USER_PAGE_BYTES);
        u32 last_page = (u32)((segment_image_end - 1ULL) / X86_64_USER_PAGE_BYTES);
        for (u32 page_index = first_page; page_index <= last_page; ++page_index) {
            if (page_index >= X86_64_USER_IMAGE_MAX_PAGES) {
                return 0U;
            }
            if ((candidate->flags & X86_64_ELF_PF_W) != 0U) {
                state->exec_user_writable_page_bitmap |= (1U << page_index);
            }
        }

        const u8 *segment = image + candidate->offset;
        for (u64 byte_index = 0ULL; byte_index < candidate->file_size; ++byte_index) {
            if (x86_64_syscall_copy_to_user_image_page(state,
                                                       segment_offset + byte_index,
                                                       segment[byte_index]) == 0U) {
                return 0U;
            }
        }

        if (header->entry >= candidate->virtual_address &&
            header->entry < segment_end &&
            (candidate->flags & X86_64_ELF_PF_X) != 0U) {
            entry_segment_seen = 1U;
        }
        if (segment_image_end > image_bytes) {
            image_bytes = segment_image_end;
        }
        load_seen = 1U;
    }

    if (load_seen == 0U ||
        entry_segment_seen == 0U ||
        image_bytes == 0ULL ||
        image_bytes > X86_64_USER_IMAGE_MAX_BYTES) {
        return 0U;
    }

    state->exec_entry = header->entry;
    state->exec_user_image_bytes = image_bytes;
    state->exec_user_image_page_count =
        (u32)((image_bytes + X86_64_USER_PAGE_BYTES - 1ULL) / X86_64_USER_PAGE_BYTES);
    state->exec_loaded = 1U;
    return 1U;
}

static inline void x86_64_syscall_dispatch_init(struct x86_64_syscall_dispatch_state *state,
                                                u32 current_pid)
{
    state->initialized = 1U;
    state->service_count = X86_64_SYSCALL_SERVICE_COUNT;
    state->current_pid = current_pid;
    state->dispatch_calls = 0U;
    state->pointer_validation_ok = 0U;
    state->user_buffer_accepted = 0U;
    state->kernel_pointer_rejected = 0U;
    x86_64_vfs_init(&state->vfs);
    state->file_service_ready = x86_64_vfs_ready(&state->vfs);
    state->file_count = x86_64_vfs_file_count();
    state->open_file_count = x86_64_vfs_open_count(&state->vfs);
    state->exit_ok = 0U;
    state->write_ok = 0U;
    state->write_fault_ok = 0U;
    state->write_output_enabled = 0U;
    state->read_ok = 0U;
    state->read_fault_ok = 0U;
    state->blocking_read_enabled = 0U;
    state->blocking_read_spins = 0U;
    state->get_arg_ok = 0U;
    state->getpid_ok = 0U;
    state->yield_ok = 0U;
    state->ps_ok = 0U;
    state->ps_fault_ok = 0U;
    state->wait_ok = 0U;
    state->wait_empty_ok = 0U;
    state->wait_fault_ok = 0U;
    state->desktop_status_ok = 0U;
    state->desktop_status_fault_ok = 0U;
    state->window_present_ok = 0U;
    state->ticks_ok = 0U;
    state->sleep_ticks_ok = 0U;
    state->input_status_ok = 0U;
    state->input_status_fault_ok = 0U;
    state->input_events_ok = 0U;
    state->input_events_empty_ok = 0U;
    state->input_events_fault_ok = 0U;
    state->input_event_cursor = x86_64_input_events_latest_sequence();
    state->exec_fault_ok = 0U;
    state->exec_path_found = 0U;
    state->exec_type_ok = 0U;
    state->exec_loaded = 0U;
    state->exec_process_created = 0U;
    state->exec_cr3_switch_requested = 0U;
    state->exec_spawned_pid = 0U;
    state->spawn_fault_ok = 0U;
    state->spawn_path_found = 0U;
    state->spawn_type_ok = 0U;
    state->spawn_loaded = 0U;
    state->spawn_process_created = 0U;
    state->spawn_spawned_pid = 0U;
    state->unknown_rejected = 0U;
    state->dispatcher_ok = 0U;
    state->exit_code = 0U;
    state->yield_count = 0U;
    state->exec_user_code_page = 0ULL;
    for (u32 i = 0U; i < X86_64_USER_IMAGE_MAX_PAGES; ++i) {
        state->exec_user_image_pages[i] = 0ULL;
    }
    state->exec_user_image_page_count = 0U;
    state->exec_user_writable_page_bitmap = 0U;
    state->exec_user_image_bytes = 0ULL;
    state->exec_entry = 0ULL;
    state->exec_target_cr3 = 0ULL;
    state->last_syscall = 0ULL;
    state->last_result = 0ULL;
    state->sample_user_buffer = X86_64_USER_CODE_BASE;
    state->sample_kernel_pointer = 0xFFFF800000100000ULL;
}

static inline u32 x86_64_syscall_user_path_ok(u64 user_path)
{
    if (x86_64_syscall_user_range_ok(user_path, 1ULL) == 0U) {
        return 0U;
    }

    return x86_64_syscall_user_cstring_valid(user_path, X86_64_VFS_PATH_MAX_BYTES);
}

static inline u64 x86_64_syscall_dispatch(struct x86_64_syscall_dispatch_state *state,
                                          u64 syscall_number,
                                          u64 arg0,
                                          u64 arg1,
                                          u64 arg2,
                                          u64 arg3,
                                          u64 arg4,
                                          u64 arg5)
{
    (void)arg3;
    (void)arg4;
    (void)arg5;

    state->dispatch_calls += 1U;
    state->last_syscall = syscall_number;

    switch (syscall_number) {
    case X86_64_SYSCALL_SERVICE_EXIT:
        state->exit_code = (u32)arg0;
        state->last_result = X86_64_SYSCALL_RET_OK;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_WRITE:
        if (x86_64_syscall_user_range_ok(arg1, arg2) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        if (arg0 == X86_64_SYSCALL_STDOUT || arg0 == X86_64_SYSCALL_STDERR) {
            if (state->write_output_enabled != 0U) {
                x86_64_syscall_write_serial((const char *)arg1, arg2);
            }
            state->last_result = arg2;
            return state->last_result;
        }

        state->last_result = x86_64_vfs_write(&state->vfs, arg0, (const char *)arg1, arg2);
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_OPEN:
        if (x86_64_syscall_user_path_ok(arg0) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        state->last_result = x86_64_vfs_open(&state->vfs, (const char *)arg0, arg1);
        state->open_file_count = x86_64_vfs_open_count(&state->vfs);
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_READ:
        if (x86_64_syscall_user_range_ok(arg1, arg2) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        if (arg0 == X86_64_SYSCALL_STDIN) {
            if (state->blocking_read_enabled != 0U) {
                state->last_result = x86_64_syscall_read_stdin_blocking(state, (char *)arg1, arg2);
                return state->last_result;
            }
            state->last_result = x86_64_syscall_read_stdin((char *)arg1, arg2);
            return state->last_result;
        }
        state->last_result = x86_64_vfs_read(&state->vfs, arg0, (char *)arg1, arg2);
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_CLOSE:
        state->last_result = x86_64_vfs_close(&state->vfs, arg0);
        state->open_file_count = x86_64_vfs_open_count(&state->vfs);
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_STAT:
        if (x86_64_syscall_user_path_ok(arg0) == 0U ||
            x86_64_syscall_user_range_ok(arg1, sizeof(u64)) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        state->last_result = x86_64_vfs_stat(&state->vfs, (const char *)arg0, (u64 *)arg1);
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_GET_ARG:
        if (arg0 != X86_64_SYSCALL_ARG_SHELL_MODE) {
            state->last_result = X86_64_SYSCALL_RET_EINVAL;
            return state->last_result;
        }
        if (x86_64_syscall_user_range_ok(arg1, arg2) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        if (x86_64_shell_interactive_requested == 0U) {
            state->last_result = X86_64_SYSCALL_RET_OK;
            return state->last_result;
        }
        state->last_result = x86_64_syscall_copy_string_arg((char *)arg1, arg2, "interactive");
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_EXEC: {
        state->exec_loaded = 0U;
        state->exec_entry = 0ULL;
        state->exec_process_created = 0U;
        state->exec_cr3_switch_requested = 0U;
        state->exec_spawned_pid = 0U;
        state->exec_target_cr3 = 0ULL;
        if (x86_64_syscall_user_path_ok(arg0) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }

        const u8 *image = (const u8 *)0;
        u64 image_size = 0ULL;
        u64 node_type = 0ULL;
        u64 resolve_result = x86_64_vfs_resolve(&state->vfs, (const char *)arg0,
                                                &image, &image_size, &node_type);
        if (resolve_result != X86_64_VFS_RET_OK) {
            state->last_result = resolve_result;
            return state->last_result;
        }

        state->exec_path_found = 1U;
        if (node_type != X86_64_VFS_NODE_EXECUTABLE) {
            state->last_result = X86_64_SYSCALL_RET_ENOSYS;
            return state->last_result;
        }

        state->exec_type_ok = 1U;
        if (x86_64_syscall_load_exec_elf(state, image, image_size) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EINVAL;
            return state->last_result;
        }

        state->exec_spawned_pid = x86_64_process_create_user_image((const char *)arg0,
                                                                   (const u8 *)state->exec_user_code_page,
                                                                   state->exec_user_image_bytes,
                                                                   state->exec_entry,
                                                                   state->exec_user_image_page_count,
                                                                   state->exec_user_writable_page_bitmap);
        if (state->exec_spawned_pid == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EINVAL;
            return state->last_result;
        }

        const struct x86_64_process_smoke_state *process_state = x86_64_process_get_smoke_state();
        if (process_state != (const struct x86_64_process_smoke_state *)0) {
            state->exec_target_cr3 = process_state->last_user_cr3;
            state->exec_cr3_switch_requested = (state->exec_target_cr3 != 0ULL) ? 1U : 0U;
        }

        state->exec_process_created = 1U;
        state->last_result = X86_64_SYSCALL_RET_OK;
        return state->last_result;
    }

    case X86_64_SYSCALL_SERVICE_SPAWN:
        state->spawn_fault_ok = 0U;
        state->spawn_path_found = 0U;
        state->spawn_type_ok = 0U;
        state->spawn_loaded = 0U;
        state->spawn_process_created = 0U;
        state->spawn_spawned_pid = 0U;
        state->exec_loaded = 0U;
        state->exec_entry = 0ULL;
        state->exec_target_cr3 = 0ULL;
        state->exec_cr3_switch_requested = 0U;

        if (x86_64_syscall_user_path_ok(arg0) == 0U) {
            state->spawn_fault_ok = 1U;
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }

        const u8 *spawn_image = (const u8 *)0;
        u64 spawn_image_size = 0ULL;
        u64 spawn_node_type = 0ULL;
        u64 spawn_resolve_result = x86_64_vfs_resolve(&state->vfs,
                                                      (const char *)arg0,
                                                      &spawn_image,
                                                      &spawn_image_size,
                                                      &spawn_node_type);
        if (spawn_resolve_result != X86_64_VFS_RET_OK) {
            state->last_result = spawn_resolve_result;
            return state->last_result;
        }

        state->spawn_path_found = 1U;
        if (spawn_node_type != X86_64_VFS_NODE_EXECUTABLE) {
            state->last_result = X86_64_SYSCALL_RET_ENOSYS;
            return state->last_result;
        }

        state->spawn_type_ok = 1U;
        if (x86_64_syscall_load_exec_elf(state, spawn_image, spawn_image_size) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EINVAL;
            return state->last_result;
        }

        state->spawn_loaded = 1U;
        state->spawn_spawned_pid = x86_64_process_create_user_image((const char *)arg0,
                                                                    (const u8 *)state->exec_user_code_page,
                                                                    state->exec_user_image_bytes,
                                                                    state->exec_entry,
                                                                    state->exec_user_image_page_count,
                                                                    state->exec_user_writable_page_bitmap);
        if (state->spawn_spawned_pid == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EINVAL;
            return state->last_result;
        }

        state->spawn_process_created = 1U;
        state->last_result = (u64)state->spawn_spawned_pid;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_GETPID:
        state->last_result = (u64)state->current_pid;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_YIELD:
        state->yield_count += 1U;
        state->last_result = X86_64_SYSCALL_RET_OK;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_PS:
        if (x86_64_syscall_user_range_ok(arg0, arg1) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        state->last_result = x86_64_process_snapshot((char *)arg0, arg1);
        state->ps_ok = (state->last_result != 0ULL) ? 1U : 0U;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_WAIT: {
        if (x86_64_syscall_user_range_ok(arg0, sizeof(u32)) == 0U ||
            x86_64_syscall_user_range_ok(arg1, sizeof(u32)) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }

        u32 child_pid = 0U;
        u32 exit_code = 0U;
        if (x86_64_process_wait_child(state->current_pid, &child_pid, &exit_code) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_ENOENT;
            return state->last_result;
        }

        *((u32 *)arg0) = child_pid;
        *((u32 *)arg1) = exit_code;
        state->wait_ok = 1U;
        state->last_result = X86_64_SYSCALL_RET_OK;
        return state->last_result;
    }

    case X86_64_SYSCALL_SERVICE_DESKTOP_STATUS:
        if (x86_64_syscall_user_range_ok(arg0, arg1) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        state->last_result = x86_64_desktop_services_snapshot((char *)arg0, arg1);
        state->desktop_status_ok = (state->last_result != 0ULL) ? 1U : 0U;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_WINDOW_PRESENT:
        state->last_result = x86_64_desktop_services_present_system_window();
        state->window_present_ok = (state->last_result == 1ULL) ? 1U : 0U;
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_TICKS: {
        struct x86_64_timer_state timer;
        x86_64_timer_get_state(&timer);
        state->ticks_ok = timer.timer_ok;
        state->last_result = (u64)timer.ticks;
        return state->last_result;
    }

    case X86_64_SYSCALL_SERVICE_SLEEP_TICKS: {
        if (arg0 > 1000ULL) {
            state->last_result = X86_64_SYSCALL_RET_EINVAL;
            return state->last_result;
        }
        struct x86_64_timer_state before;
        struct x86_64_timer_state after;
        x86_64_timer_get_state(&before);
        x86_64_timer_wait_for_ticks((u32)arg0);
        x86_64_timer_get_state(&after);
        state->sleep_ticks_ok =
            (after.ticks >= before.ticks && (after.ticks - before.ticks) >= (u32)arg0) ? 1U : 0U;
        state->last_result = (u64)(after.ticks - before.ticks);
        return state->last_result;
    }

    case X86_64_SYSCALL_SERVICE_INPUT_STATUS:
        if (x86_64_syscall_user_range_ok(arg0, arg1) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        state->last_result = x86_64_syscall_format_input_status((char *)arg0, arg1);
        state->input_status_ok = (state->last_result != 0ULL) ? 1U : 0U;
        return state->last_result;

        case X86_64_SYSCALL_SERVICE_INPUT_EVENTS: {
        u64 max_events = arg1;
        u64 byte_count = max_events * (u64)sizeof(struct x86_64_input_event);
        if (max_events > X86_64_INPUT_EVENT_QUEUE_CAPACITY ||
            byte_count / sizeof(struct x86_64_input_event) != max_events) {
            state->last_result = X86_64_SYSCALL_RET_EINVAL;
            return state->last_result;
        }
        if (x86_64_syscall_user_range_ok(arg0, byte_count) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        state->last_result = x86_64_input_events_read_from(&state->input_event_cursor,
                                                           (struct x86_64_input_event *)arg0,
                                                           max_events);
        state->input_events_ok = 1U;
        state->input_events_empty_ok = (state->last_result == 0ULL) ? 1U : 0U;
        return state->last_result;
    }

    case X86_64_SYSCALL_SERVICE_MKDIR:
        if (x86_64_syscall_user_path_ok(arg0) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        state->last_result = x86_64_vfs_mkdir(&state->vfs, (const char *)arg0);
        state->file_count = x86_64_vfs_file_count();
        return state->last_result;

    case X86_64_SYSCALL_SERVICE_UNLINK:
        if (x86_64_syscall_user_path_ok(arg0) == 0U) {
            state->last_result = X86_64_SYSCALL_RET_EFAULT;
            return state->last_result;
        }
        state->last_result = x86_64_vfs_unlink(&state->vfs, (const char *)arg0);
        state->file_count = x86_64_vfs_file_count();
        return state->last_result;

    default:
        state->last_result = X86_64_SYSCALL_RET_ENOSYS;
        return state->last_result;
    }
}

static inline u64 x86_64_syscall_dispatch_frame(struct x86_64_syscall_dispatch_state *state,
                                                struct x86_64_syscall_frame *frame)
{
    frame->exit_requested = 0U;
    frame->cr3_switch_requested = 0U;
    frame->target_cr3 = 0ULL;
    frame->result = x86_64_syscall_dispatch(state,
                                            frame->number,
                                            frame->arg0,
                                            frame->arg1,
                                            frame->arg2,
                                            frame->arg3,
                                            frame->arg4,
                                            frame->arg5);

    if (frame->number == X86_64_SYSCALL_SERVICE_EXEC &&
        frame->result == X86_64_SYSCALL_RET_OK &&
        state->exec_loaded != 0U) {
        frame->user_rip = state->exec_entry;
        frame->cr3_switch_requested = state->exec_cr3_switch_requested;
        frame->target_cr3 = state->exec_target_cr3;
    }

    if (frame->number == X86_64_SYSCALL_SERVICE_EXIT && frame->result == X86_64_SYSCALL_RET_OK) {
        frame->exit_requested = 1U;
    }

    return frame->result;
}

static inline void x86_64_syscall_dispatch_run_smoke(struct x86_64_syscall_dispatch_state *state,
                                                     u32 current_pid)
{
    x86_64_syscall_dispatch_init(state, current_pid);

    state->user_buffer_accepted =
        x86_64_syscall_user_range_ok(state->sample_user_buffer, 4ULL);
    state->kernel_pointer_rejected =
        (x86_64_syscall_user_range_ok(state->sample_kernel_pointer, 4ULL) == 0U) ? 1U : 0U;
    state->pointer_validation_ok =
        ((state->user_buffer_accepted != 0U) &&
         (state->kernel_pointer_rejected != 0U)) ? 1U : 0U;

    u64 write_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_WRITE,
                                               X86_64_SYSCALL_STDOUT, state->sample_user_buffer, 4ULL,
                                               0ULL, 0ULL, 0ULL);
    state->write_ok = (write_result == 4ULL) ? 1U : 0U;

    u64 write_fault = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_WRITE,
                                              X86_64_SYSCALL_STDOUT, state->sample_kernel_pointer, 4ULL,
                                              0ULL, 0ULL, 0ULL);
    state->write_fault_ok = (write_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 read_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_READ,
                                              X86_64_SYSCALL_STDIN, state->sample_user_buffer, 4ULL,
                                              0ULL, 0ULL, 0ULL);
    state->read_ok = (read_result == X86_64_SYSCALL_RET_OK) ? 1U : 0U;

    u64 read_fault = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_READ,
                                             X86_64_SYSCALL_STDIN, state->sample_kernel_pointer, 4ULL,
                                             0ULL, 0ULL, 0ULL);
    state->read_fault_ok = (read_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 get_arg_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_GET_ARG,
                                                 X86_64_SYSCALL_ARG_SHELL_MODE,
                                                 state->sample_user_buffer, 0ULL,
                                                 0ULL, 0ULL, 0ULL);
    state->get_arg_ok = (get_arg_result == X86_64_SYSCALL_RET_OK) ? 1U : 0U;

    u64 getpid_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_GETPID,
                                                0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    state->getpid_ok = (getpid_result == (u64)current_pid) ? 1U : 0U;

    u64 yield_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_YIELD,
                                               0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    state->yield_ok = ((yield_result == X86_64_SYSCALL_RET_OK) &&
                       (state->yield_count == 1U)) ? 1U : 0U;

    u64 ps_fault = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_PS,
                                           state->sample_kernel_pointer, 32ULL, 0ULL,
                                           0ULL, 0ULL, 0ULL);
    state->ps_fault_ok = (ps_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 wait_empty = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_WAIT,
                                             state->sample_user_buffer,
                                             state->sample_user_buffer + sizeof(u32),
                                             0ULL, 0ULL, 0ULL, 0ULL);
    state->wait_empty_ok = (wait_empty == X86_64_SYSCALL_RET_ENOENT) ? 1U : 0U;

    u64 wait_fault = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_WAIT,
                                             state->sample_kernel_pointer,
                                             state->sample_user_buffer + sizeof(u32),
                                             0ULL, 0ULL, 0ULL, 0ULL);
    state->wait_fault_ok = (wait_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 desktop_status = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_DESKTOP_STATUS,
                                                 state->sample_user_buffer, 256ULL, 0ULL,
                                                 0ULL, 0ULL, 0ULL);
    state->desktop_status_ok = (desktop_status != 0ULL) ? 1U : 0U;

    u64 desktop_status_fault =
        x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_DESKTOP_STATUS,
                                state->sample_kernel_pointer, 256ULL, 0ULL,
                                0ULL, 0ULL, 0ULL);
    state->desktop_status_fault_ok =
        (desktop_status_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 window_present = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_WINDOW_PRESENT,
                                                 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    state->window_present_ok = (window_present == 1ULL) ? 1U : 0U;

    u64 ticks_before = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_TICKS,
                                               0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    u64 sleep_ticks = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_SLEEP_TICKS,
                                              1ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    u64 ticks_after = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_TICKS,
                                              0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    state->ticks_ok = (ticks_after >= ticks_before) ? 1U : 0U;
    state->sleep_ticks_ok = (sleep_ticks >= 1ULL) ? 1U : 0U;

    u64 input_status = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_INPUT_STATUS,
                                               state->sample_user_buffer, 128ULL, 0ULL,
                                               0ULL, 0ULL, 0ULL);
    state->input_status_ok = (input_status != 0ULL) ? 1U : 0U;

    u64 input_status_fault =
        x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_INPUT_STATUS,
                                state->sample_kernel_pointer, 128ULL, 0ULL,
                                0ULL, 0ULL, 0ULL);
    state->input_status_fault_ok =
        (input_status_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 input_events = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_INPUT_EVENTS,
                                               state->sample_user_buffer, 2ULL, 0ULL,
                                               0ULL, 0ULL, 0ULL);
    state->input_events_ok = (input_events <= 2ULL) ? 1U : 0U;
    state->input_events_empty_ok = (input_events == 0ULL) ? 1U : 0U;

    u64 input_events_fault =
        x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_INPUT_EVENTS,
                                state->sample_kernel_pointer, 1ULL, 0ULL,
                                0ULL, 0ULL, 0ULL);
    state->input_events_fault_ok =
        (input_events_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 exec_fault = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_EXEC,
                                             state->sample_kernel_pointer, 0ULL, 0ULL,
                                             0ULL, 0ULL, 0ULL);
    state->exec_fault_ok = (exec_fault == X86_64_SYSCALL_RET_EFAULT) ? 1U : 0U;

    u64 unknown_result = x86_64_syscall_dispatch(state, 0xFFULL,
                                                 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    state->unknown_rejected = (unknown_result == X86_64_SYSCALL_RET_ENOSYS) ? 1U : 0U;

    u64 exit_result = x86_64_syscall_dispatch(state, X86_64_SYSCALL_SERVICE_EXIT,
                                              7ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    state->exit_ok = ((exit_result == X86_64_SYSCALL_RET_OK) &&
                      (state->exit_code == 7U)) ? 1U : 0U;

    state->dispatcher_ok =
        ((state->initialized != 0U) &&
         (state->service_count == X86_64_SYSCALL_SERVICE_COUNT) &&
         (state->pointer_validation_ok != 0U) &&
         (state->file_service_ready != 0U) &&
         (state->file_count == x86_64_vfs_file_count()) &&
         (state->write_ok != 0U) &&
         (state->write_fault_ok != 0U) &&
         (state->read_ok != 0U) &&
         (state->read_fault_ok != 0U) &&
         (state->get_arg_ok != 0U) &&
         (state->getpid_ok != 0U) &&
         (state->yield_ok != 0U) &&
         (state->ps_fault_ok != 0U) &&
         (state->wait_empty_ok != 0U) &&
         (state->wait_fault_ok != 0U) &&
         (state->desktop_status_ok != 0U) &&
         (state->desktop_status_fault_ok != 0U) &&
         (state->window_present_ok != 0U) &&
         (state->ticks_ok != 0U) &&
         (state->sleep_ticks_ok != 0U) &&
         (state->input_status_ok != 0U) &&
         (state->input_status_fault_ok != 0U) &&
         (state->input_events_ok != 0U) &&
         (state->input_events_empty_ok != 0U) &&
         (state->input_events_fault_ok != 0U) &&
         (state->exec_fault_ok != 0U) &&
         (state->unknown_rejected != 0U) &&
         (state->exit_ok != 0U) &&
         (state->dispatch_calls == 23U)) ? 1U : 0U;
}

static inline void x86_64_syscall_dispatch_report(const struct x86_64_syscall_dispatch_state *state)
{
    x86_64_serial_write_line("x86_64 syscall dispatcher online");
    x86_64_serial_write_u32("Syscall dispatcher initialized: ", state->initialized);
    x86_64_serial_write_u32("Syscall dispatcher service count: ", state->service_count);
    x86_64_serial_write_u32("Syscall dispatcher calls: ", state->dispatch_calls);
    x86_64_serial_write_hex64("Syscall sample user buffer: 0x", state->sample_user_buffer);
    x86_64_serial_write_hex64("Syscall sample kernel pointer: 0x", state->sample_kernel_pointer);
    x86_64_serial_write_u32("Syscall user buffer accepted: ", state->user_buffer_accepted);
    x86_64_serial_write_u32("Syscall kernel pointer rejected: ", state->kernel_pointer_rejected);
    x86_64_serial_write_u32("Syscall pointer validation ok: ", state->pointer_validation_ok);
    x86_64_serial_write_u32("Syscall file service ready: ", state->file_service_ready);
    x86_64_serial_write_u32("Syscall file count: ", state->file_count);
    x86_64_serial_write_u32("Syscall open files: ", state->open_file_count);
    x86_64_serial_write_u32("Syscall write dispatch ok: ", state->write_ok);
    x86_64_serial_write_u32("Syscall write fault ok: ", state->write_fault_ok);
    x86_64_serial_write_u32("Syscall read dispatch ok: ", state->read_ok);
    x86_64_serial_write_u32("Syscall read fault ok: ", state->read_fault_ok);
    x86_64_serial_write_u32("Syscall blocking read enabled: ", state->blocking_read_enabled);
    x86_64_serial_write_u32("Syscall blocking read spins: ", state->blocking_read_spins);
    x86_64_serial_write_u32("Syscall get_arg dispatch ok: ", state->get_arg_ok);
    x86_64_serial_write_u32("Syscall getpid dispatch ok: ", state->getpid_ok);
    x86_64_serial_write_u32("Syscall yield dispatch ok: ", state->yield_ok);
    x86_64_serial_write_u32("Syscall ps dispatch ok: ", state->ps_ok);
    x86_64_serial_write_u32("Syscall ps fault ok: ", state->ps_fault_ok);
    x86_64_serial_write_u32("Syscall wait dispatch ok: ", state->wait_ok);
    x86_64_serial_write_u32("Syscall wait empty ok: ", state->wait_empty_ok);
    x86_64_serial_write_u32("Syscall wait fault ok: ", state->wait_fault_ok);
    x86_64_serial_write_u32("Syscall desktop status ok: ", state->desktop_status_ok);
    x86_64_serial_write_u32("Syscall desktop status fault ok: ", state->desktop_status_fault_ok);
    x86_64_serial_write_u32("Syscall window present ok: ", state->window_present_ok);
    x86_64_serial_write_u32("Syscall ticks ok: ", state->ticks_ok);
    x86_64_serial_write_u32("Syscall sleep ticks ok: ", state->sleep_ticks_ok);
    x86_64_serial_write_u32("Syscall input status ok: ", state->input_status_ok);
    x86_64_serial_write_u32("Syscall input status fault ok: ", state->input_status_fault_ok);
    x86_64_serial_write_u32("Syscall input events ok: ", state->input_events_ok);
    x86_64_serial_write_u32("Syscall input events empty ok: ", state->input_events_empty_ok);
    x86_64_serial_write_u32("Syscall input events fault ok: ", state->input_events_fault_ok);
    x86_64_serial_write_u32("Syscall exec fault ok: ", state->exec_fault_ok);
    x86_64_serial_write_u32("Syscall exec path found: ", state->exec_path_found);
    x86_64_serial_write_u32("Syscall exec type ok: ", state->exec_type_ok);
    x86_64_serial_write_u32("Syscall exec loaded: ", state->exec_loaded);
    x86_64_serial_write_u32("Syscall exec process created: ", state->exec_process_created);
    x86_64_serial_write_u32("Syscall exec CR3 switch requested: ", state->exec_cr3_switch_requested);
    x86_64_serial_write_u32("Syscall exec spawned pid: ", state->exec_spawned_pid);
    x86_64_serial_write_hex64("Syscall exec entry: 0x", state->exec_entry);
    x86_64_serial_write_hex64("Syscall exec target CR3: 0x", state->exec_target_cr3);
    x86_64_serial_write_u32("Syscall unknown rejected: ", state->unknown_rejected);
    x86_64_serial_write_u32("Syscall exit dispatch ok: ", state->exit_ok);
    x86_64_serial_write_u32("Syscall exit code: ", state->exit_code);
    x86_64_serial_write_u32("Syscall yield count: ", state->yield_count);
    x86_64_serial_write_hex64("Syscall last number: 0x", state->last_syscall);
    x86_64_serial_write_hex64("Syscall last result: 0x", state->last_result);
    x86_64_serial_write_u32("Syscall dispatcher ok: ", state->dispatcher_ok);
}

#endif
