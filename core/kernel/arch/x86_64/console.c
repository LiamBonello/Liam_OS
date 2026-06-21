#include "console.h"

#define VGA_TEXT_BUFFER ((volatile u16 *)0xB8000ULL)
#define VGA_WIDTH 80ULL
#define VGA_HEIGHT 25ULL
#define VGA_ATTR 0x0FULL

#define COM1 0x3F8U

static u16 vga_entry(char c)
{
    return (u16)c | (u16)(VGA_ATTR << 8);
}

static char hex_digit(u32 value)
{
    value &= 0xFU;
    return value < 10U ? (char)('0' + value) : (char)('A' + value - 10U);
}

static void outb(u16 port, u8 value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static u8 inb(u16 port)
{
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_write_char(char c)
{
    for (u32 i = 0; i < 100000U; ++i) {
        if ((inb(COM1 + 5U) & 0x20U) != 0U) {
            outb(COM1, (u8)c);
            return;
        }
    }
}

void x86_64_console_init(void)
{
    volatile u16 *vga = VGA_TEXT_BUFFER;
    for (usize i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) {
        vga[i] = vga_entry(' ');
    }
}

void x86_64_console_write_at(const char *message, usize row, usize column)
{
    if (row >= VGA_HEIGHT || column >= VGA_WIDTH) {
        return;
    }

    volatile u16 *vga = VGA_TEXT_BUFFER + (row * VGA_WIDTH) + column;
    usize remaining = VGA_WIDTH - column;

    while (*message != '\0' && remaining > 0ULL) {
        *vga++ = vga_entry(*message++);
        --remaining;
    }
}

void x86_64_console_write_hex32(usize row, usize column, const char *label, u32 value)
{
    char buffer[9];

    for (u32 i = 0; i < 8U; ++i) {
        u32 shift = 28U - (i * 4U);
        buffer[i] = hex_digit(value >> shift);
    }
    buffer[8] = '\0';

    x86_64_console_write_at(label, row, column);
    while (*label != '\0') {
        ++column;
        ++label;
    }
    x86_64_console_write_at(buffer, row, column);
}

void x86_64_serial_init(void)
{
    outb(COM1 + 1U, 0x00U);
    outb(COM1 + 3U, 0x80U);
    outb(COM1 + 0U, 0x03U);
    outb(COM1 + 1U, 0x00U);
    outb(COM1 + 3U, 0x03U);
    outb(COM1 + 2U, 0xC7U);
    outb(COM1 + 4U, 0x0BU);
}

void x86_64_serial_write(const char *message)
{
    while (*message != '\0') {
        serial_write_char(*message++);
    }
}

void x86_64_serial_write_line(const char *message)
{
    x86_64_serial_write(message);
    serial_write_char('\r');
    serial_write_char('\n');
}

void x86_64_serial_write_hex32(const char *label, u32 value)
{
    char buffer[9];

    for (u32 i = 0; i < 8U; ++i) {
        u32 shift = 28U - (i * 4U);
        buffer[i] = hex_digit(value >> shift);
    }
    buffer[8] = '\0';

    x86_64_serial_write(label);
    x86_64_serial_write_line(buffer);
}
