#include "framebuffer.h"

static void zero_state(struct x86_64_framebuffer_state *state)
{
    u8 *bytes = (u8 *)state;
    for (usize i = 0; i < sizeof(*state); ++i) {
        bytes[i] = 0U;
    }
}

static u32 field_mask(u8 bits)
{
    if (bits == 0U) {
        return 0U;
    }
    if (bits >= 32U) {
        return 0xFFFFFFFFU;
    }
    return (1U << bits) - 1U;
}

static u32 rgb8_to_field(u8 value, u8 shift, u8 bits)
{
    u32 field;

    if (bits == 0U) {
        return 0U;
    }

    if (bits >= 8U) {
        field = ((u32)value) << (bits - 8U);
    } else {
        field = ((u32)value) >> (8U - bits);
    }

    return (field & field_mask(bits)) << shift;
}

static u32 pixel_offset_ok(const struct x86_64_framebuffer_state *state, u32 x, u32 y, u64 *offset)
{
    if (state == (const struct x86_64_framebuffer_state *)0 ||
        state->initialized == 0U ||
        x >= state->width ||
        y >= state->height ||
        state->bytes_per_pixel == 0U) {
        return 0U;
    }

    *offset = ((u64)y * (u64)state->pitch) + ((u64)x * (u64)state->bytes_per_pixel);
    return ((*offset + (u64)state->bytes_per_pixel) <= state->buffer_bytes) ? 1U : 0U;
}

static u32 read_pixel(const struct x86_64_framebuffer_state *state, u32 x, u32 y)
{
    u64 offset = 0ULL;
    if (pixel_offset_ok(state, x, y, &offset) == 0U) {
        return 0U;
    }

    volatile u8 *pixel = ((volatile u8 *)state->address) + offset;
    if (state->bytes_per_pixel == 4U) {
        return *((volatile u32 *)pixel);
    }

    return ((u32)pixel[0]) | (((u32)pixel[1]) << 8U) | (((u32)pixel[2]) << 16U);
}

void x86_64_framebuffer_init(struct x86_64_framebuffer_state *state,
                             const struct x86_64_boot_summary *boot_info,
                             const struct x86_64_paging_builder_state *paging_builder)
{
    zero_state(state);

    if (boot_info == (const struct x86_64_boot_summary *)0 ||
        paging_builder == (const struct x86_64_paging_builder_state *)0) {
        return;
    }

    state->boot_info_ready = boot_info->framebuffer_rgb_format_ok;
    state->mapping_ready =
        ((paging_builder->builder_ok != 0U) &&
         (paging_builder->framebuffer_entry_ok != 0U)) ? 1U : 0U;
    state->format_ready =
        ((boot_info->framebuffer_bpp == 24U || boot_info->framebuffer_bpp == 32U) &&
         boot_info->framebuffer_pitch >=
             (boot_info->framebuffer_width * ((u32)boot_info->framebuffer_bpp / 8U))) ? 1U : 0U;

    if (state->boot_info_ready == 0U || state->mapping_ready == 0U || state->format_ready == 0U) {
        return;
    }

    state->address = paging_builder->framebuffer_virtual_address;
    state->width = boot_info->framebuffer_width;
    state->height = boot_info->framebuffer_height;
    state->pitch = boot_info->framebuffer_pitch;
    state->bpp = (u32)boot_info->framebuffer_bpp;
    state->bytes_per_pixel = state->bpp / 8U;
    state->buffer_bytes = (u64)state->pitch * (u64)state->height;
    state->red_shift = boot_info->framebuffer_red_field_position;
    state->red_bits = boot_info->framebuffer_red_mask_size;
    state->green_shift = boot_info->framebuffer_green_field_position;
    state->green_bits = boot_info->framebuffer_green_mask_size;
    state->blue_shift = boot_info->framebuffer_blue_field_position;
    state->blue_bits = boot_info->framebuffer_blue_mask_size;
    state->background_color = x86_64_framebuffer_rgb(state, 12U, 18U, 28U);
    state->accent_color = x86_64_framebuffer_rgb(state, 88U, 190U, 255U);
    state->initialized = 1U;
}

u32 x86_64_framebuffer_rgb(const struct x86_64_framebuffer_state *state, u8 red, u8 green, u8 blue)
{
    if (state == (const struct x86_64_framebuffer_state *)0) {
        return 0U;
    }

    return rgb8_to_field(red, state->red_shift, state->red_bits) |
           rgb8_to_field(green, state->green_shift, state->green_bits) |
           rgb8_to_field(blue, state->blue_shift, state->blue_bits);
}

u32 x86_64_framebuffer_put_pixel(const struct x86_64_framebuffer_state *state, u32 x, u32 y, u32 color)
{
    u64 offset = 0ULL;
    if (pixel_offset_ok(state, x, y, &offset) == 0U) {
        return 0U;
    }

    volatile u8 *pixel = ((volatile u8 *)state->address) + offset;
    if (state->bytes_per_pixel == 4U) {
        *((volatile u32 *)pixel) = color;
    } else {
        pixel[0] = (u8)(color & 0xFFU);
        pixel[1] = (u8)((color >> 8U) & 0xFFU);
        pixel[2] = (u8)((color >> 16U) & 0xFFU);
    }

    return 1U;
}

u32 x86_64_framebuffer_fill_rect(const struct x86_64_framebuffer_state *state,
                                 u32 x,
                                 u32 y,
                                 u32 width,
                                 u32 height,
                                 u32 color)
{
    if (state == (const struct x86_64_framebuffer_state *)0 ||
        state->initialized == 0U ||
        x >= state->width ||
        y >= state->height) {
        return 0U;
    }

    u32 end_x = x + width;
    u32 end_y = y + height;
    if (end_x > state->width || end_x < x) {
        end_x = state->width;
    }
    if (end_y > state->height || end_y < y) {
        end_y = state->height;
    }

    for (u32 row = y; row < end_y; ++row) {
        for (u32 column = x; column < end_x; ++column) {
            if (x86_64_framebuffer_put_pixel(state, column, row, color) == 0U) {
                return 0U;
            }
        }
    }

    return 1U;
}

void x86_64_framebuffer_run_smoke(struct x86_64_framebuffer_state *state)
{
    if (state == (struct x86_64_framebuffer_state *)0 || state->initialized == 0U) {
        return;
    }

    state->clear_ok = x86_64_framebuffer_fill_rect(state, 0U, 0U, state->width, state->height,
                                                   state->background_color);
    state->fill_ok = x86_64_framebuffer_fill_rect(state, 0U, 0U, state->width, 48U,
                                                  state->accent_color);
    state->pixel_ok = x86_64_framebuffer_put_pixel(state, 16U, 16U, state->accent_color);
    state->sampled_color = read_pixel(state, 16U, 16U);
    state->smoke_ok =
        ((state->clear_ok != 0U) &&
         (state->fill_ok != 0U) &&
         (state->pixel_ok != 0U) &&
         (state->sampled_color == state->accent_color)) ? 1U : 0U;
}
