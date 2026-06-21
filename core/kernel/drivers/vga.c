#include "vga.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER_ADDRESS 0xB8000

static volatile uint16_t* const VGA_BUFFER = (volatile uint16_t*)VGA_BUFFER_ADDRESS;

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static uint8_t current_color = 0x0F;

static uint8_t make_color(vga_color_t foreground, vga_color_t background) {
    return (uint8_t)foreground | ((uint8_t)background << 4);
}

static uint16_t make_entry(char character, uint8_t color) {
    return (uint16_t)character | ((uint16_t)color << 8);
}

static void put_entry_at(char character, uint8_t color, uint32_t x, uint32_t y) {
    VGA_BUFFER[y * VGA_WIDTH + x] = make_entry(character, color);
}

static void newline(void) {
    cursor_x = 0;

    if (cursor_y < VGA_HEIGHT - 1) {
        cursor_y++;
        return;
    }

    for (uint32_t y = 1; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[(y - 1) * VGA_WIDTH + x] = VGA_BUFFER[y * VGA_WIDTH + x];
        }
    }

    for (uint32_t x = 0; x < VGA_WIDTH; x++) {
        put_entry_at(' ', current_color, x, VGA_HEIGHT - 1);
    }
}

static void put_char(char character) {
    if (character == '\n') {
        newline();
        return;
    }

    put_entry_at(character, current_color, cursor_x, cursor_y);
    cursor_x++;

    if (cursor_x >= VGA_WIDTH) {
        newline();
    }
}

void vga_initialize(void) {
    cursor_x = 0;
    cursor_y = 0;
    current_color = make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_clear(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            put_entry_at(' ', current_color, x, y);
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}

void vga_set_color(vga_color_t foreground, vga_color_t background) {
    current_color = make_color(foreground, background);
}

void vga_write_char(char character) {
    put_char(character);
}

void vga_backspace(void) {
    if (cursor_x == 0 && cursor_y == 0) {
        return;
    }

    if (cursor_x == 0) {
        cursor_y--;
        cursor_x = VGA_WIDTH - 1;
    } else {
        cursor_x--;
    }

    put_entry_at(' ', current_color, cursor_x, cursor_y);
}

void vga_write(const char* text) {
    for (uint32_t i = 0; text[i] != '\0'; i++) {
        put_char(text[i]);
    }
}

void vga_write_line(const char* text) {
    vga_write(text);
    put_char('\n');
}

void vga_write_at(const char* text, uint32_t x, uint32_t y) {
    uint32_t old_x = cursor_x;
    uint32_t old_y = cursor_y;

    cursor_x = x;
    cursor_y = y;

    vga_write(text);

    cursor_x = old_x;
    cursor_y = old_y;
}

void vga_write_hex_u32(uint32_t value) {
    const char* digits = "0123456789ABCDEF";

    vga_write("0x");

    for (int shift = 28; shift >= 0; shift -= 4) {
        uint8_t nibble = (uint8_t)((value >> shift) & 0xF);
        char character = digits[nibble];
        put_char(character);
    }
}

void vga_write_u32(uint32_t value) {
    char buffer[11];
    uint32_t index = 0;

    if (value == 0) {
        put_char('0');
        return;
    }

    while (value > 0 && index < 10) {
        buffer[index] = (char)('0' + (value % 10));
        value /= 10;
        index++;
    }

    while (index > 0) {
        index--;
        put_char(buffer[index]);
    }
}