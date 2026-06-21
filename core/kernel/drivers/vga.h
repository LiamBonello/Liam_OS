#ifndef LIAM_OS_VGA_H
#define LIAM_OS_VGA_H

#include "../core/types.h"


typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15
} vga_color_t;

void vga_initialize(void);
void vga_clear(void);
void vga_set_color(vga_color_t foreground, vga_color_t background);
void vga_write_char(char character);
void vga_backspace(void);
void vga_write(const char* text);
void vga_write_line(const char* text);
void vga_write_at(const char* text, uint32_t x, uint32_t y);
void vga_write_hex_u32(uint32_t value);
void vga_write_u32(uint32_t value);

#endif