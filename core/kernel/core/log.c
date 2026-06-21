#include "log.h"
#include "../drivers/vga.h"

static void log_with_prefix(
    const char* prefix,
    const char* message,
    vga_color_t prefix_color
) {
    vga_set_color(prefix_color, VGA_COLOR_BLACK);
    vga_write(prefix);

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write_line(message);
}

void log_info(const char* message) {
    log_with_prefix("[INFO] ", message, VGA_COLOR_LIGHT_CYAN);
}

void log_success(const char* message) {
    log_with_prefix("[ OK ] ", message, VGA_COLOR_LIGHT_GREEN);
}

void log_warning(const char* message) {
    log_with_prefix("[WARN] ", message, VGA_COLOR_LIGHT_BROWN);
}

void log_error(const char* message) {
    log_with_prefix("[FAIL] ", message, VGA_COLOR_LIGHT_RED);
}

void log_debug(const char* message) {
#ifdef LIAM_OS_DEBUG
    log_with_prefix("[DBG ] ", message, VGA_COLOR_DARK_GREY);
#else
    (void)message;
#endif
}

void log_info_hex_u32(const char* label, uint32_t value) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write("[INFO] ");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write(label);
    vga_write(": ");
    vga_write_hex_u32(value);
    vga_write_line("");
}

void log_info_u32(const char* label, uint32_t value) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write("[INFO] ");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write(label);
    vga_write(": ");
    vga_write_u32(value);
    vga_write_line("");
}

void log_debug_hex_u32(const char* label, uint32_t value) {
#ifdef LIAM_OS_DEBUG
    vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    vga_write("[DBG ] ");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write(label);
    vga_write(": ");
    vga_write_hex_u32(value);
    vga_write_line("");
#else
    (void)label;
    (void)value;
#endif
}

void log_debug_u32(const char* label, uint32_t value) {
#ifdef LIAM_OS_DEBUG
    vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    vga_write("[DBG ] ");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write(label);
    vga_write(": ");
    vga_write_u32(value);
    vga_write_line("");
#else
    (void)label;
    (void)value;
#endif
}