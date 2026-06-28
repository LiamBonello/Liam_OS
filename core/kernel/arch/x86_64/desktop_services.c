#include "desktop_services.h"

#include "idt.h"
#include "vfs.h"

#define X86_64_DESKTOP_QUANTUM_TICKS 5U
#define X86_64_DESKTOP_WINDOW_X 96U
#define X86_64_DESKTOP_WINDOW_Y 96U
#define X86_64_DESKTOP_WINDOW_WIDTH 420U
#define X86_64_DESKTOP_WINDOW_HEIGHT 260U

static struct x86_64_desktop_services_state desktop_state;
static struct x86_64_framebuffer_state desktop_framebuffer;

static void clear_bytes(void *ptr, usize size)
{
    u8 *bytes = (u8 *)ptr;
    for (usize i = 0; i < size; ++i) {
        bytes[i] = 0U;
    }
}

static u64 append_char(char *buffer, u64 size, u64 offset, char value)
{
    if (offset + 1ULL < size) {
        buffer[offset] = value;
    }

    return offset + 1ULL;
}

static u64 append_string(char *buffer, u64 size, u64 offset, const char *value)
{
    if (value == (const char *)0) {
        return offset;
    }

    for (u64 i = 0ULL; value[i] != '\0'; ++i) {
        offset = append_char(buffer, size, offset, value[i]);
    }

    return offset;
}

static u64 append_u32(char *buffer, u64 size, u64 offset, u32 value)
{
    char digits[10];
    u32 count = 0U;

    if (value == 0U) {
        return append_char(buffer, size, offset, '0');
    }

    while (value != 0U && count < (u32)sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (count > 0U) {
        --count;
        offset = append_char(buffer, size, offset, digits[count]);
    }

    return offset;
}

static void refresh_desktop_state(void)
{
    struct x86_64_timer_state timer;
    struct x86_64_keyboard_state keyboard;

    x86_64_timer_get_state(&timer);
    x86_64_keyboard_get_state(&keyboard);

    desktop_state.timer_ready = timer.timer_ok;
    desktop_state.scheduler_tick_ready =
        ((timer.timer_ok != 0U) &&
         (timer.frequency_hz != 0U) &&
         (timer.interrupts_enabled_after != 0U)) ? 1U : 0U;
    desktop_state.scheduler_observed_ticks = timer.ticks;
    desktop_state.scheduler_observed_slices =
        timer.ticks / desktop_state.scheduler_quantum_ticks;
    desktop_state.input_ready = keyboard.keyboard_ok;
    desktop_state.input_buffer_capacity = keyboard.buffer_capacity;
    desktop_state.input_buffered_chars = keyboard.buffered_chars;
    desktop_state.storage_readonly_vfs_ready = 1U;
    desktop_state.storage_session_ready = x86_64_vfs_session_storage_ready();
    desktop_state.storage_persistent_ready = 0U;
    desktop_state.storage_mounts = 1U;
    desktop_state.graphics_ready = desktop_framebuffer.initialized;
    desktop_state.window_service_ready =
        ((desktop_state.scheduler_tick_ready != 0U) &&
         (desktop_state.input_ready != 0U) &&
         (desktop_state.storage_readonly_vfs_ready != 0U) &&
         (desktop_state.storage_session_ready != 0U) &&
         (desktop_state.graphics_ready != 0U)) ? 1U : 0U;
}

void x86_64_desktop_services_init(const struct x86_64_framebuffer_state *framebuffer)
{
    clear_bytes(&desktop_state, sizeof(desktop_state));
    clear_bytes(&desktop_framebuffer, sizeof(desktop_framebuffer));

    desktop_state.initialized = 1U;
    desktop_state.scheduler_quantum_ticks = X86_64_DESKTOP_QUANTUM_TICKS;

    if (framebuffer != (const struct x86_64_framebuffer_state *)0) {
        desktop_framebuffer = *framebuffer;
    }

    refresh_desktop_state();
}

void x86_64_desktop_services_get_state(struct x86_64_desktop_services_state *state)
{
    if (state == (struct x86_64_desktop_services_state *)0) {
        return;
    }

    refresh_desktop_state();
    *state = desktop_state;
}

u64 x86_64_desktop_services_snapshot(char *buffer, u64 size)
{
    if (buffer == (char *)0 || size == 0ULL || desktop_state.initialized == 0U) {
        return 0ULL;
    }

    refresh_desktop_state();

    u64 offset = 0ULL;
    offset = append_string(buffer, size, offset, "desktop services\n");
    offset = append_string(buffer, size, offset, "scheduler-ready ");
    offset = append_u32(buffer, size, offset, desktop_state.scheduler_tick_ready);
    offset = append_char(buffer, size, offset, '\n');
    offset = append_string(buffer, size, offset, "scheduler-ticks ");
    offset = append_u32(buffer, size, offset, desktop_state.scheduler_observed_ticks);
    offset = append_char(buffer, size, offset, '\n');
    offset = append_string(buffer, size, offset, "scheduler-slices ");
    offset = append_u32(buffer, size, offset, desktop_state.scheduler_observed_slices);
    offset = append_char(buffer, size, offset, '\n');
    offset = append_string(buffer, size, offset, "input-ready ");
    offset = append_u32(buffer, size, offset, desktop_state.input_ready);
    offset = append_char(buffer, size, offset, '\n');
    offset = append_string(buffer, size, offset, "input-buffered ");
    offset = append_u32(buffer, size, offset, desktop_state.input_buffered_chars);
    offset = append_char(buffer, size, offset, '\n');
    offset = append_string(buffer, size, offset, "storage-readonly-vfs ");
    offset = append_u32(buffer, size, offset, desktop_state.storage_readonly_vfs_ready);
    offset = append_char(buffer, size, offset, '\n');
    offset = append_string(buffer, size, offset, "storage-session ");
    offset = append_u32(buffer, size, offset, desktop_state.storage_session_ready);
    offset = append_char(buffer, size, offset, '\n');
    offset = append_string(buffer, size, offset, "storage-persistent ");
    offset = append_u32(buffer, size, offset, desktop_state.storage_persistent_ready);
    offset = append_char(buffer, size, offset, '\n');
    offset = append_string(buffer, size, offset, "graphics-ready ");
    offset = append_u32(buffer, size, offset, desktop_state.graphics_ready);
    offset = append_char(buffer, size, offset, '\n');
    offset = append_string(buffer, size, offset, "window-service-ready ");
    offset = append_u32(buffer, size, offset, desktop_state.window_service_ready);
    offset = append_char(buffer, size, offset, '\n');
    offset = append_string(buffer, size, offset, "window-present-calls ");
    offset = append_u32(buffer, size, offset, desktop_state.window_present_calls);
    offset = append_char(buffer, size, offset, '\n');

    if (offset < size) {
        buffer[offset] = '\0';
    } else {
        buffer[size - 1ULL] = '\0';
    }

    desktop_state.snapshot_ok = (offset > 0ULL && offset < size) ? 1U : 0U;
    return offset;
}

u64 x86_64_desktop_services_present_demo_window(void)
{
    refresh_desktop_state();

    if (desktop_state.window_service_ready == 0U) {
        desktop_state.present_ok = 0U;
        return 0ULL;
    }

    u32 shadow = x86_64_framebuffer_rgb(&desktop_framebuffer, 22U, 28U, 36U);
    u32 body = x86_64_framebuffer_rgb(&desktop_framebuffer, 226U, 232U, 240U);
    u32 title = x86_64_framebuffer_rgb(&desktop_framebuffer, 56U, 118U, 214U);
    u32 edge = x86_64_framebuffer_rgb(&desktop_framebuffer, 20U, 24U, 32U);

    u32 shadow_ok = x86_64_framebuffer_fill_rect(&desktop_framebuffer,
                                                 X86_64_DESKTOP_WINDOW_X + 8U,
                                                 X86_64_DESKTOP_WINDOW_Y + 8U,
                                                 X86_64_DESKTOP_WINDOW_WIDTH,
                                                 X86_64_DESKTOP_WINDOW_HEIGHT,
                                                 shadow);
    u32 body_ok = x86_64_framebuffer_fill_rect(&desktop_framebuffer,
                                               X86_64_DESKTOP_WINDOW_X,
                                               X86_64_DESKTOP_WINDOW_Y,
                                               X86_64_DESKTOP_WINDOW_WIDTH,
                                               X86_64_DESKTOP_WINDOW_HEIGHT,
                                               body);
    u32 title_ok = x86_64_framebuffer_fill_rect(&desktop_framebuffer,
                                                X86_64_DESKTOP_WINDOW_X,
                                                X86_64_DESKTOP_WINDOW_Y,
                                                X86_64_DESKTOP_WINDOW_WIDTH,
                                                36U,
                                                title);
    u32 edge_ok = x86_64_framebuffer_fill_rect(&desktop_framebuffer,
                                               X86_64_DESKTOP_WINDOW_X,
                                               X86_64_DESKTOP_WINDOW_Y,
                                               X86_64_DESKTOP_WINDOW_WIDTH,
                                               2U,
                                               edge);

    desktop_state.window_present_calls += 1U;
    desktop_state.last_window_x = X86_64_DESKTOP_WINDOW_X;
    desktop_state.last_window_y = X86_64_DESKTOP_WINDOW_Y;
    desktop_state.last_window_width = X86_64_DESKTOP_WINDOW_WIDTH;
    desktop_state.last_window_height = X86_64_DESKTOP_WINDOW_HEIGHT;
    desktop_state.present_ok =
        ((shadow_ok != 0U) && (body_ok != 0U) && (title_ok != 0U) && (edge_ok != 0U)) ? 1U : 0U;

    return (desktop_state.present_ok != 0U) ? 1ULL : 0ULL;
}

void x86_64_desktop_services_run_smoke(struct x86_64_desktop_services_state *state)
{
    char buffer[512];
    u64 snapshot_bytes = x86_64_desktop_services_snapshot(buffer, sizeof(buffer));
    u64 present_result = x86_64_desktop_services_present_demo_window();

    refresh_desktop_state();
    desktop_state.snapshot_ok = (snapshot_bytes > 0ULL && snapshot_bytes < sizeof(buffer)) ? 1U : 0U;
    desktop_state.present_ok = (present_result == 1ULL) ? 1U : 0U;
    desktop_state.smoke_ok =
        ((desktop_state.initialized != 0U) &&
         (desktop_state.scheduler_tick_ready != 0U) &&
         (desktop_state.input_ready != 0U) &&
         (desktop_state.storage_readonly_vfs_ready != 0U) &&
         (desktop_state.storage_session_ready != 0U) &&
         (desktop_state.graphics_ready != 0U) &&
         (desktop_state.window_service_ready != 0U) &&
         (desktop_state.snapshot_ok != 0U) &&
         (desktop_state.present_ok != 0U)) ? 1U : 0U;

    if (state != (struct x86_64_desktop_services_state *)0) {
        *state = desktop_state;
    }
}
