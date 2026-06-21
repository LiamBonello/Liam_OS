#include "console.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289U

void kernel_main_x86_64(u32 boot_magic, u32 boot_info)
{
    x86_64_console_init();
    x86_64_serial_init();

    x86_64_console_write_at("Liam_OS x86_64 C kernel entry online", 0, 0);
    x86_64_console_write_at("Stage: boot state + early console/serial", 1, 0);

    x86_64_serial_write_line("Liam_OS x86_64 C kernel entry online");
    x86_64_serial_write_line("Stage: boot state + early console/serial");

    if (boot_magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        x86_64_console_write_at("Multiboot2 magic: ok", 2, 0);
        x86_64_serial_write_line("Multiboot2 magic: ok");
    } else {
        x86_64_console_write_at("Multiboot2 magic: bad", 2, 0);
        x86_64_serial_write_line("Multiboot2 magic: bad");
    }

    x86_64_console_write_hex32(3, 0, "Boot info pointer: 0x", boot_info);
    x86_64_serial_write_hex32("Boot magic: 0x", boot_magic);
    x86_64_serial_write_hex32("Boot info pointer: 0x", boot_info);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
