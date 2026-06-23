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

static char hex_digit(u64 value)
{
    value &= 0xFULL;
    return value < 10ULL ? (char)('0' + value) : (char)('A' + value - 10ULL);
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

static u32 serial_received(void)
{
    return ((inb(COM1 + 5U) & 0x01U) != 0U) ? 1U : 0U;
}

static void make_hex32(char *buffer, u32 value)
{
    for (u32 i = 0; i < 8U; ++i) {
        u32 shift = 28U - (i * 4U);
        buffer[i] = hex_digit(value >> shift);
    }
    buffer[8] = '\0';
}

static void make_hex64(char *buffer, u64 value)
{
    for (u32 i = 0; i < 16U; ++i) {
        u32 shift = 60U - (i * 4U);
        buffer[i] = hex_digit(value >> shift);
    }
    buffer[16] = '\0';
}

static void make_u32(char *buffer, u32 value)
{
    static const u32 powers[] = {
        1000000000U,
        100000000U,
        10000000U,
        1000000U,
        100000U,
        10000U,
        1000U,
        100U,
        10U,
        1U,
    };
    u32 index = 0;
    u32 started = 0;

    for (u32 i = 0; i < 10U; ++i) {
        u32 digit = 0;
        while (value >= powers[i]) {
            value -= powers[i];
            ++digit;
        }

        if (digit != 0U || started != 0U || powers[i] == 1U) {
            buffer[index++] = (char)('0' + digit);
            started = 1U;
        }
    }

    buffer[index] = '\0';
}

static usize append_label(char *buffer, const char *label)
{
    usize index = 0;
    while (label[index] != '\0') {
        buffer[index] = label[index];
        ++index;
    }
    return index;
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

void x86_64_console_write_u32(usize row, usize column, const char *label, u32 value)
{
    char buffer[64];
    usize index = append_label(buffer, label);
    make_u32(buffer + index, value);
    x86_64_console_write_at(buffer, row, column);
}

void x86_64_console_write_hex32(usize row, usize column, const char *label, u32 value)
{
    char buffer[64];
    usize index = append_label(buffer, label);
    make_hex32(buffer + index, value);
    x86_64_console_write_at(buffer, row, column);
}

void x86_64_console_write_hex64(usize row, usize column, const char *label, u64 value)
{
    char buffer[80];
    usize index = append_label(buffer, label);
    make_hex64(buffer + index, value);
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

void x86_64_serial_write_u32(const char *label, u32 value)
{
    char buffer[64];
    usize index = append_label(buffer, label);
    make_u32(buffer + index, value);
    x86_64_serial_write_line(buffer);
}

void x86_64_serial_write_hex32(const char *label, u32 value)
{
    char buffer[64];
    usize index = append_label(buffer, label);
    make_hex32(buffer + index, value);
    x86_64_serial_write_line(buffer);
}

void x86_64_serial_write_hex64(const char *label, u64 value)
{
    char buffer[80];
    usize index = append_label(buffer, label);
    make_hex64(buffer + index, value);
    x86_64_serial_write_line(buffer);
}

u64 x86_64_serial_read(char *buffer, u64 size)
{
    if (buffer == (char *)0 || size == 0ULL) {
        return 0ULL;
    }

    u64 bytes = 0ULL;
    while (bytes < size && serial_received() != 0U) {
        buffer[bytes] = (char)inb(COM1);
        bytes += 1ULL;
    }

    return bytes;
}
