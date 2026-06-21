#include "panic.h"
#include "../drivers/vga.h"

void kernel_panic(const char* message) {
    __asm__ volatile ("cli");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_clear();

    vga_write_line("LIAM OS KERNEL PANIC");
    vga_write_line("");

    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write_line("A fatal kernel error occurred.");
    vga_write_line("");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write("Reason: ");
    vga_write_line(message);
    vga_write_line("");

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_write_line("The system has been halted to prevent damage.");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}