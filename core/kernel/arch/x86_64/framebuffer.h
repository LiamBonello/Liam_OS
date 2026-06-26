#ifndef LIAM_OS_X86_64_FRAMEBUFFER_H
#define LIAM_OS_X86_64_FRAMEBUFFER_H

#include "boot_info.h"
#include "paging_builder.h"
#include "types.h"

struct x86_64_framebuffer_state {
    u64 address;
    u64 buffer_bytes;
    u32 width;
    u32 height;
    u32 pitch;
    u32 bpp;
    u32 bytes_per_pixel;
    u32 background_color;
    u32 accent_color;
    u32 sampled_color;
    u8 red_shift;
    u8 red_bits;
    u8 green_shift;
    u8 green_bits;
    u8 blue_shift;
    u8 blue_bits;
    u32 boot_info_ready;
    u32 mapping_ready;
    u32 format_ready;
    u32 initialized;
    u32 clear_ok;
    u32 fill_ok;
    u32 pixel_ok;
    u32 smoke_ok;
};

void x86_64_framebuffer_init(struct x86_64_framebuffer_state *state,
                             const struct x86_64_boot_summary *boot_info,
                             const struct x86_64_paging_builder_state *paging_builder);
u32 x86_64_framebuffer_rgb(const struct x86_64_framebuffer_state *state, u8 red, u8 green, u8 blue);
u32 x86_64_framebuffer_put_pixel(const struct x86_64_framebuffer_state *state, u32 x, u32 y, u32 color);
u32 x86_64_framebuffer_fill_rect(const struct x86_64_framebuffer_state *state,
                                 u32 x,
                                 u32 y,
                                 u32 width,
                                 u32 height,
                                 u32 color);
void x86_64_framebuffer_run_smoke(struct x86_64_framebuffer_state *state);

#endif
