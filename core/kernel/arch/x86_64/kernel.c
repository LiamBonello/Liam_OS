#include "boot_info.h"
#include "console.h"
#include "idt.h"

static void report_boot_summary(const struct x86_64_boot_summary *summary)
{
    if (summary->multiboot2_valid != 0U) {
        x86_64_console_write_at("Multiboot2: ok", 2, 0);
        x86_64_serial_write_line("Multiboot2: ok");
    } else {
        x86_64_console_write_at("Multiboot2: invalid", 2, 0);
        x86_64_serial_write_line("Multiboot2: invalid");
        x86_64_serial_write_hex32("Boot magic: 0x", summary->magic);
        x86_64_serial_write_hex32("Boot info pointer: 0x", summary->boot_info_addr);
        return;
    }

    if (summary->bootloader_name_found != 0U) {
        x86_64_console_write_at("Bootloader:", 3, 0);
        x86_64_console_write_at(summary->bootloader_name, 3, 12);
        x86_64_serial_write("Bootloader: ");
        x86_64_serial_write_line(summary->bootloader_name);
    } else {
        x86_64_console_write_at("Bootloader: unknown", 3, 0);
        x86_64_serial_write_line("Bootloader: unknown");
    }

    x86_64_console_write_hex32(4, 0, "Boot info pointer: 0x", summary->boot_info_addr);
    x86_64_console_write_u32(5, 0, "Boot info bytes: ", summary->total_size);
    x86_64_serial_write_hex32("Boot info pointer: 0x", summary->boot_info_addr);
    x86_64_serial_write_u32("Boot info bytes: ", summary->total_size);

    if (summary->basic_meminfo_found != 0U) {
        x86_64_console_write_u32(6, 0, "Lower KiB: ", summary->mem_lower_kib);
        x86_64_console_write_u32(7, 0, "Upper KiB: ", summary->mem_upper_kib);
        x86_64_serial_write_u32("Lower KiB: ", summary->mem_lower_kib);
        x86_64_serial_write_u32("Upper KiB: ", summary->mem_upper_kib);
    } else {
        x86_64_console_write_at("Basic memory info: missing", 6, 0);
        x86_64_serial_write_line("Basic memory info: missing");
    }

    if (summary->mmap_found != 0U) {
        x86_64_console_write_u32(8, 0, "Memory map entries: ", summary->mmap_entry_count);
        x86_64_console_write_hex64(9, 0, "Usable bytes: 0x", summary->usable_memory_bytes);
        x86_64_serial_write_u32("Memory map entries: ", summary->mmap_entry_count);
        x86_64_serial_write_hex64("Usable bytes: 0x", summary->usable_memory_bytes);
    } else {
        x86_64_console_write_at("Memory map: missing", 8, 0);
        x86_64_serial_write_line("Memory map: missing");
    }
}

void kernel_main_x86_64(u32 boot_magic, u32 boot_info)
{
    struct x86_64_boot_summary summary;

    x86_64_console_init();
    x86_64_serial_init();
    x86_64_idt_init();

    x86_64_boot_info_parse(boot_magic, boot_info, &summary);

    x86_64_console_write_at("Liam_OS x86_64 kernel diagnostics", 0, 0);
    x86_64_console_write_at("Stage: IDT exceptions + boot memory summary", 1, 0);

    x86_64_serial_write_line("Liam_OS x86_64 kernel diagnostics");
    x86_64_serial_write_line("Stage: IDT exceptions + boot memory summary");
    x86_64_serial_write_line("x86_64 IDT: exceptions installed");

    report_boot_summary(&summary);
    x86_64_console_write_at("IDT: exceptions installed", 10, 0);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
