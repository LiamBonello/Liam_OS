#ifndef LIAM_OS_X86_64_DESKTOP_SERVICES_H
#define LIAM_OS_X86_64_DESKTOP_SERVICES_H

#include "framebuffer.h"
#include "types.h"

struct x86_64_desktop_services_state {
    u32 initialized;
    u32 timer_ready;
    u32 scheduler_tick_ready;
    u32 scheduler_quantum_ticks;
    u32 scheduler_observed_ticks;
    u32 scheduler_observed_slices;
    u32 input_ready;
    u32 input_buffer_capacity;
    u32 input_buffered_chars;
    u32 input_event_capacity;
    u32 input_event_queued;
    u32 input_event_drops;
    u32 input_event_oldest_sequence;
    u32 input_event_latest_sequence;
    u32 input_event_service_ready;
    u32 storage_readonly_vfs_ready;
    u32 storage_session_ready;
    u32 storage_persistent_ready;
    u32 storage_mounts;
    u32 graphics_ready;
    u32 window_service_ready;
    u32 window_present_calls;
    u32 last_window_x;
    u32 last_window_y;
    u32 last_window_width;
    u32 last_window_height;
    u32 snapshot_ok;
    u32 present_ok;
    u32 smoke_ok;
};

void x86_64_desktop_services_init(const struct x86_64_framebuffer_state *framebuffer);
void x86_64_desktop_services_get_state(struct x86_64_desktop_services_state *state);
u64 x86_64_desktop_services_snapshot(char *buffer, u64 size);
u64 x86_64_desktop_services_present_system_window(void);
void x86_64_desktop_services_run_smoke(struct x86_64_desktop_services_state *state);

#endif
