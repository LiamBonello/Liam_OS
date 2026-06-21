#include "boot_context.h"
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

static void report_memory_layout(const struct x86_64_memory_layout *layout)
{
    x86_64_console_write_at("IDT: exceptions installed", 10, 0);
    x86_64_console_write_hex64(11, 0, "Kernel start: 0x", layout->kernel_start);
    x86_64_console_write_hex64(12, 0, "Kernel end: 0x", layout->kernel_end);
    x86_64_console_write_hex64(13, 0, "Kernel bytes: 0x", layout->kernel_size_bytes);
    x86_64_console_write_hex64(14, 0, "Identity map bytes: 0x", layout->bootstrap_identity_map_bytes);

    x86_64_serial_write_line("x86_64 IDT: exceptions installed");
    x86_64_serial_write_hex64("Kernel start: 0x", layout->kernel_start);
    x86_64_serial_write_hex64("Kernel end: 0x", layout->kernel_end);
    x86_64_serial_write_hex64("Kernel bytes: 0x", layout->kernel_size_bytes);
    x86_64_serial_write_hex64("Identity map bytes: 0x", layout->bootstrap_identity_map_bytes);
}

static void report_pmm_plan(const struct x86_64_pmm_plan *plan)
{
    x86_64_console_write_u32(15, 0, "PMM usable regions: ", plan->usable_region_count);
    x86_64_console_write_hex64(16, 0, "PMM first page: 0x", plan->first_free_page);
    x86_64_console_write_hex64(17, 0, "PMM pages: 0x", plan->managed_pages);
    x86_64_console_write_hex64(18, 0, "PMM bytes: 0x", plan->managed_bytes);
    x86_64_console_write_hex64(19, 0, "Reserved below: 0x", plan->reserved_below);

    x86_64_serial_write_u32("PMM usable regions: ", plan->usable_region_count);
    x86_64_serial_write_hex64("PMM first page: 0x", plan->first_free_page);
    x86_64_serial_write_hex64("PMM pages: 0x", plan->managed_pages);
    x86_64_serial_write_hex64("PMM bytes: 0x", plan->managed_bytes);
    x86_64_serial_write_hex64("Reserved below: 0x", plan->reserved_below);
}

void kernel_main_x86_64(u32 boot_magic, u32 boot_info)
{
    struct x86_64_boot_context context;

    x86_64_console_init();
    x86_64_serial_init();
    x86_64_idt_init();

    x86_64_boot_context_init(boot_magic, boot_info, &context);

    x86_64_console_write_at("Liam_OS x86_64 kernel diagnostics", 0, 0);
    x86_64_console_write_at("Stage: PMM plan + boot context", 1, 0);

    x86_64_serial_write_line("Liam_OS x86_64 kernel diagnostics");
    x86_64_serial_write_line("Stage: PMM plan + boot context");

    report_boot_summary(&context.boot_info);
    report_memory_layout(&context.memory_layout);
    report_pmm_plan(&context.pmm_plan);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
