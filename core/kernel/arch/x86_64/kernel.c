#include "boot_context.h"
#include "console.h"
#include "cpuid.h"
#include "desktop_services.h"
#include "framebuffer.h"
#include "gdt.h"
#include "heap.h"
#include "higher_half_probe.h"
#include "idt.h"
#include "paging.h"
#include "paging_builder.h"
#include "paging_plan.h"
#include "pci.h"
#include "pmm.h"
#include "runtime.h"
#include "storage_hw.h"
#include "tss.h"
#include "user_mode.h"

static void report_boot_summary(const struct x86_64_boot_summary *summary)
{
    if (summary->multiboot2_valid != 0U) {
        x86_64_console_write_at("Multiboot2: ok", 2, 0);
        x86_64_serial_write_line("Multiboot2: ok");
    } else {
        x86_64_console_write_at("Multiboot2: invalid", 2, 0);
        x86_64_serial_write_line("Multiboot2: invalid");
        x86_64_serial_write_hex32("Boot magic: 0x", summary->magic);
        x86_64_serial_write_hex32("Boot info pointer: 0x", summary->boot_info_addr);
        return;
    }

    if (summary->bootloader_name_found != 0U) {
        x86_64_console_write_at("Bootloader:", 3, 0);
        x86_64_console_write_at(summary->bootloader_name, 3, 12);
        x86_64_serial_write("Bootloader: ");
        x86_64_serial_write_line(summary->bootloader_name);
    } else {
        x86_64_console_write_at("Bootloader: unknown", 3, 0);
        x86_64_serial_write_line("Bootloader: unknown");
    }

    x86_64_console_write_hex32(4, 0, "Boot info pointer: 0x", summary->boot_info_addr);
    x86_64_console_write_u32(5, 0, "Boot info bytes: ", summary->total_size);
    x86_64_serial_write_hex32("Boot info pointer: 0x", summary->boot_info_addr);
    x86_64_serial_write_u32("Boot info bytes: ", summary->total_size);

    if (summary->basic_meminfo_found != 0U) {
        x86_64_console_write_u32(6, 0, "Lower KiB: ", summary->mem_lower_kib);
        x86_64_console_write_u32(7, 0, "Upper KiB: ", summary->mem_upper_kib);
        x86_64_serial_write_u32("Lower KiB: ", summary->mem_lower_kib);
        x86_64_serial_write_u32("Upper KiB: ", summary->mem_upper_kib);
    } else {
        x86_64_console_write_at("Basic memory info: missing", 6, 0);
        x86_64_serial_write_line("Basic memory info: missing");
    }

    if (summary->mmap_found != 0U) {
        x86_64_console_write_u32(8, 0, "Memory map entries: ", summary->mmap_entry_count);
        x86_64_console_write_hex64(9, 0, "Usable bytes: 0x", summary->usable_memory_bytes);
        x86_64_serial_write_u32("Memory map entries: ", summary->mmap_entry_count);
        x86_64_serial_write_hex64("Usable bytes: 0x", summary->usable_memory_bytes);
    } else {
        x86_64_console_write_at("Memory map: missing", 8, 0);
        x86_64_serial_write_line("Memory map: missing");
    }

    x86_64_serial_write_line("x86_64 framebuffer diagnostics");
    x86_64_serial_write_u32("Framebuffer found: ", summary->framebuffer_found);
    x86_64_serial_write_hex64("Framebuffer address: 0x", summary->framebuffer_addr);
    x86_64_serial_write_u32("Framebuffer pitch: ", summary->framebuffer_pitch);
    x86_64_serial_write_u32("Framebuffer width: ", summary->framebuffer_width);
    x86_64_serial_write_u32("Framebuffer height: ", summary->framebuffer_height);
    x86_64_serial_write_u32("Framebuffer bpp: ", (u32)summary->framebuffer_bpp);
    x86_64_serial_write_u32("Framebuffer type: ", (u32)summary->framebuffer_type);
    x86_64_serial_write_u32("Framebuffer RGB format ok: ", summary->framebuffer_rgb_format_ok);
    x86_64_serial_write_u32("Framebuffer red shift: ", (u32)summary->framebuffer_red_field_position);
    x86_64_serial_write_u32("Framebuffer red bits: ", (u32)summary->framebuffer_red_mask_size);
    x86_64_serial_write_u32("Framebuffer green shift: ", (u32)summary->framebuffer_green_field_position);
    x86_64_serial_write_u32("Framebuffer green bits: ", (u32)summary->framebuffer_green_mask_size);
    x86_64_serial_write_u32("Framebuffer blue shift: ", (u32)summary->framebuffer_blue_field_position);
    x86_64_serial_write_u32("Framebuffer blue bits: ", (u32)summary->framebuffer_blue_mask_size);
}

static void report_framebuffer_summary(const struct x86_64_boot_summary *summary)
{
    x86_64_serial_write_line("x86_64 framebuffer summary");
    x86_64_serial_write_u32("Framebuffer found: ", summary->framebuffer_found);
    x86_64_serial_write_hex64("Framebuffer address: 0x", summary->framebuffer_addr);
    x86_64_serial_write_u32("Framebuffer pitch: ", summary->framebuffer_pitch);
    x86_64_serial_write_u32("Framebuffer width: ", summary->framebuffer_width);
    x86_64_serial_write_u32("Framebuffer height: ", summary->framebuffer_height);
    x86_64_serial_write_u32("Framebuffer bpp: ", (u32)summary->framebuffer_bpp);
    x86_64_serial_write_u32("Framebuffer type: ", (u32)summary->framebuffer_type);
    x86_64_serial_write_u32("Framebuffer RGB format ok: ", summary->framebuffer_rgb_format_ok);
}

static void report_framebuffer_mapping_summary(const struct x86_64_paging_builder_state *state)
{
    x86_64_serial_write_line("x86_64 framebuffer mapping summary");
    x86_64_serial_write_hex64("Framebuffer virtual address: 0x", state->framebuffer_virtual_address);
    x86_64_serial_write_u32("Framebuffer huge pages: ", state->framebuffer_huge_pages);
    x86_64_serial_write_u32("Framebuffer mapping ready: ", state->framebuffer_mapped);
}

static void report_framebuffer_surface(const struct x86_64_framebuffer_state *state)
{
    x86_64_serial_write_line("x86_64 framebuffer surface online");
    x86_64_serial_write_hex64("Framebuffer surface address: 0x", state->address);
    x86_64_serial_write_u32("Framebuffer surface width: ", state->width);
    x86_64_serial_write_u32("Framebuffer surface height: ", state->height);
    x86_64_serial_write_u32("Framebuffer surface pitch: ", state->pitch);
    x86_64_serial_write_u32("Framebuffer surface bpp: ", state->bpp);
    x86_64_serial_write_u32("Framebuffer surface bytes per pixel: ", state->bytes_per_pixel);
    x86_64_serial_write_u32("Framebuffer surface boot info ready: ", state->boot_info_ready);
    x86_64_serial_write_u32("Framebuffer surface mapping ready: ", state->mapping_ready);
    x86_64_serial_write_u32("Framebuffer surface format ready: ", state->format_ready);
    x86_64_serial_write_u32("Framebuffer surface ready: ", state->initialized);
    x86_64_serial_write_u32("Framebuffer clear ok: ", state->clear_ok);
    x86_64_serial_write_u32("Framebuffer fill ok: ", state->fill_ok);
    x86_64_serial_write_u32("Framebuffer pixel ok: ", state->pixel_ok);
    x86_64_serial_write_hex32("Framebuffer sampled color: 0x", state->sampled_color);
    x86_64_serial_write_u32("Framebuffer smoke ok: ", state->smoke_ok);
}

static void report_desktop_services(const struct x86_64_desktop_services_state *state)
{
    x86_64_serial_write_line("x86_64 desktop services online");
    x86_64_serial_write_u32("Desktop services initialized: ", state->initialized);
    x86_64_serial_write_u32("Desktop scheduler ticks ready: ", state->scheduler_tick_ready);
    x86_64_serial_write_u32("Desktop scheduler quantum ticks: ", state->scheduler_quantum_ticks);
    x86_64_serial_write_u32("Desktop scheduler observed ticks: ", state->scheduler_observed_ticks);
    x86_64_serial_write_u32("Desktop scheduler observed slices: ", state->scheduler_observed_slices);
    x86_64_serial_write_u32("Desktop input ready: ", state->input_ready);
    x86_64_serial_write_u32("Desktop input buffer capacity: ", state->input_buffer_capacity);
    x86_64_serial_write_u32("Desktop input events ready: ", state->input_event_service_ready);
    x86_64_serial_write_u32("Desktop input event capacity: ", state->input_event_capacity);
    x86_64_serial_write_u32("Desktop input event queued: ", state->input_event_queued);
    x86_64_serial_write_u32("Desktop storage readonly VFS ready: ", state->storage_readonly_vfs_ready);
    x86_64_serial_write_u32("Desktop session storage ready: ", state->storage_session_ready);
    x86_64_serial_write_u32("Desktop persistent storage ready: ", state->storage_persistent_ready);
    x86_64_serial_write_u32("Desktop graphics ready: ", state->graphics_ready);
    x86_64_serial_write_u32("Desktop window service ready: ", state->window_service_ready);
    x86_64_serial_write_u32("Desktop window present calls: ", state->window_present_calls);
    x86_64_serial_write_u32("Desktop services snapshot ok: ", state->snapshot_ok);
    x86_64_serial_write_u32("Desktop services present ok: ", state->present_ok);
    x86_64_serial_write_u32("Desktop services smoke ok: ", state->smoke_ok);
}

static void report_cpu_state(const struct x86_64_cpuid_state *state)
{
    x86_64_console_write_u32(10, 0, "CPU baseline ok: ", state->baseline_ok);

    x86_64_serial_write_line("x86_64 CPU baseline diagnostics");
    x86_64_serial_write_u32("CPU CPUID available: ", state->cpuid_available);
    x86_64_serial_write("CPU vendor: ");
    x86_64_serial_write_line(state->vendor);
    x86_64_serial_write_hex32("CPU max basic leaf: 0x", state->max_basic_leaf);
    x86_64_serial_write_hex32("CPU max extended leaf: 0x", state->max_extended_leaf);
    x86_64_serial_write_u32("CPU FPU: ", state->has_fpu);
    x86_64_serial_write_u32("CPU MSR: ", state->has_msr);
    x86_64_serial_write_u32("CPU APIC: ", state->has_apic);
    x86_64_serial_write_u32("CPU SSE: ", state->has_sse);
    x86_64_serial_write_u32("CPU SSE2: ", state->has_sse2);
    x86_64_serial_write_u32("CPU SYSCALL/SYSRET: ", state->has_fast_syscall);
    x86_64_serial_write_u32("CPU NX: ", state->has_nx);
    x86_64_serial_write_u32("CPU 1G pages: ", state->has_1g_pages);
    x86_64_serial_write_u32("CPU long mode: ", state->has_long_mode);
    x86_64_serial_write_u32("CPU hypervisor: ", state->hypervisor_present);
    x86_64_serial_write_u32("CPU baseline ok: ", state->baseline_ok);
}

static void report_pci_state(const struct x86_64_pci_state *state)
{
    x86_64_serial_write_line("x86_64 PCI bus discovery online");
    x86_64_serial_write_u32("PCI config I/O ready: ", state->config_io_ready);
    x86_64_serial_write_u32("PCI devices found: ", state->devices_found);
    x86_64_serial_write_u32("PCI multifunction devices: ", state->multifunction_devices);
    x86_64_serial_write_u32("PCI storage controllers: ", state->storage_controllers);
    x86_64_serial_write_u32("PCI AHCI controllers: ", state->ahci_controllers);
    x86_64_serial_write_u32("PCI NVMe controllers: ", state->nvme_controllers);
}

static void report_storage_hw_state(const struct x86_64_storage_hw_state *state)
{
    x86_64_serial_write_line("x86_64 storage hardware inventory online");
    x86_64_serial_write_u32("Storage hardware initialized: ", state->initialized);
    x86_64_serial_write_u32("Storage PCI ready: ", state->pci_ready);
    x86_64_serial_write_u32("Storage PCI devices: ", state->pci_devices);
    x86_64_serial_write_u32("Storage controllers: ", state->storage_controllers);
    x86_64_serial_write_u32("Storage AHCI controllers: ", state->ahci_controllers);
    x86_64_serial_write_u32("Storage NVMe controllers: ", state->nvme_controllers);
    x86_64_serial_write_u32("Storage AHCI controller found: ", state->ahci_controller_found);
    x86_64_serial_write_hex32("Storage AHCI BAR5 raw: 0x", state->ahci_bar5_raw);
    x86_64_serial_write_hex64("Storage AHCI MMIO base: 0x", state->ahci_mmio_base);
    x86_64_serial_write_u32("Storage AHCI MMIO BAR ready: ", state->ahci_mmio_bar_ready);
    x86_64_serial_write_u32("Storage AHCI MMIO mapped: ", state->ahci_mmio_mapped);
    x86_64_serial_write_u32("Storage block driver ready: ", state->block_driver_ready);
    x86_64_serial_write_u32("Storage persistent ready: ", state->persistent_ready);
}

static void report_idt_state(const struct x86_64_idt_state *state)
{
    x86_64_console_write_u32(11, 0, "IDT IST gates: ", state->ist_gates_ok);

    x86_64_serial_write_line("x86_64 IDT: exceptions installed");
    x86_64_serial_write_hex64("IDT base: 0x", state->idtr_base);
    x86_64_serial_write_u32("IDT limit: ", state->idtr_limit);
    x86_64_serial_write_u32("IDT exception gates: ", state->exception_gates);
    x86_64_serial_write_u32("IDT NMI vector: ", state->nmi_vector);
    x86_64_serial_write_u32("IDT NMI IST: ", state->nmi_ist);
    x86_64_serial_write_u32("IDT NMI present: ", state->nmi_present);
    x86_64_serial_write_u32("IDT NMI IST ok: ", state->nmi_ist_ok);
    x86_64_serial_write_u32("IDT double fault vector: ", state->double_fault_vector);
    x86_64_serial_write_u32("IDT double fault IST: ", state->double_fault_ist);
    x86_64_serial_write_u32("IDT double fault present: ", state->double_fault_present);
    x86_64_serial_write_u32("IDT double fault IST ok: ", state->double_fault_ist_ok);
    x86_64_serial_write_u32("IDT page fault vector: ", state->page_fault_vector);
    x86_64_serial_write_u32("IDT page fault IST: ", state->page_fault_ist);
    x86_64_serial_write_u32("IDT page fault present: ", state->page_fault_present);
    x86_64_serial_write_u32("IDT page fault IST ok: ", state->page_fault_ist_ok);
    x86_64_serial_write_u32("IDT IST gates ok: ", state->ist_gates_ok);
}

static void report_memory_layout(const struct x86_64_memory_layout *layout)
{
    x86_64_console_write_hex64(12, 0, "Kernel start: 0x", layout->kernel_start);
    x86_64_console_write_hex64(13, 0, "Kernel end: 0x", layout->kernel_end);
    x86_64_console_write_hex64(14, 0, "Kernel bytes: 0x", layout->kernel_size_bytes);
    x86_64_console_write_hex64(15, 0, "Identity map bytes: 0x", layout->bootstrap_identity_map_bytes);

    x86_64_serial_write_hex64("Kernel start: 0x", layout->kernel_start);
    x86_64_serial_write_hex64("Kernel end: 0x", layout->kernel_end);
    x86_64_serial_write_hex64("Kernel bytes: 0x", layout->kernel_size_bytes);
    x86_64_serial_write_hex64("Identity map bytes: 0x", layout->bootstrap_identity_map_bytes);
}

static void report_pmm_plan(const struct x86_64_pmm_plan *plan)
{
    x86_64_console_write_u32(16, 0, "PMM usable regions: ", plan->usable_region_count);
    x86_64_console_write_hex64(17, 0, "PMM first page: 0x", plan->first_free_page);
    x86_64_console_write_hex64(18, 0, "PMM pages: 0x", plan->managed_pages);
    x86_64_console_write_hex64(19, 0, "PMM bytes: 0x", plan->managed_bytes);
    x86_64_console_write_hex64(20, 0, "Reserved below: 0x", plan->reserved_below);

    x86_64_serial_write_u32("PMM usable regions: ", plan->usable_region_count);
    x86_64_serial_write_hex64("PMM first page: 0x", plan->first_free_page);
    x86_64_serial_write_hex64("PMM pages: 0x", plan->managed_pages);
    x86_64_serial_write_hex64("PMM bytes: 0x", plan->managed_bytes);
    x86_64_serial_write_hex64("Reserved below: 0x", plan->reserved_below);
}

static void report_pmm_allocator(void)
{
    const struct x86_64_pmm_state *state = x86_64_pmm_get_state();
    u64 page = x86_64_pmm_alloc_page();
    u32 free_ok = x86_64_pmm_free_page(page);

    x86_64_console_write_u32(21, 0, "PMM tracked pages: ", state->tracked_pages);
    x86_64_console_write_hex64(22, 0, "PMM smoke page: 0x", page);
    x86_64_console_write_u32(23, 0, "PMM smoke free: ", free_ok);

    x86_64_serial_write_line("x86_64 PMM allocator online");
    x86_64_serial_write_u32("PMM tracked pages: ", state->tracked_pages);
    x86_64_serial_write_u32("PMM free pages: ", state->free_pages);
    x86_64_serial_write_u32("PMM dropped pages: ", state->dropped_pages);
    x86_64_serial_write_u32("PMM duplicate rejects: ", state->duplicate_free_rejects);
    x86_64_serial_write_hex64("PMM lowest page: 0x", state->lowest_page);
    x86_64_serial_write_hex64("PMM highest page: 0x", state->highest_page);
    x86_64_serial_write_hex64("PMM smoke page: 0x", page);
    x86_64_serial_write_u32("PMM smoke free: ", free_ok);
}

static void report_paging_state(const struct x86_64_paging_state *state)
{
    x86_64_serial_write_line("x86_64 bootstrap paging online");
    x86_64_serial_write_hex64("Paging CR3: 0x", state->cr3);
    x86_64_serial_write_hex64("Paging PML4: 0x", state->pml4_table);
    x86_64_serial_write_hex64("Paging PDPT: 0x", state->pdpt_table);
    x86_64_serial_write_hex64("Paging PD: 0x", state->pd_table);
    x86_64_serial_write_u32("Paging PML4 present: ", state->pml4_present);
    x86_64_serial_write_u32("Paging PDPT present: ", state->pdpt_present);
    x86_64_serial_write_u32("Paging huge pages: ", state->huge_pages_present);
    x86_64_serial_write_hex64("Paging first huge page: 0x", state->first_huge_page);
    x86_64_serial_write_hex64("Paging last huge page: 0x", state->last_huge_page);
    x86_64_serial_write_hex64("Paging identity bytes: 0x", state->identity_map_bytes);
}

static void report_paging_plan(const struct x86_64_paging_plan *plan)
{
    x86_64_serial_write_line("x86_64 virtual memory plan online");
    x86_64_serial_write_hex64("VM identity window bytes: 0x", plan->identity_window_bytes);
    x86_64_serial_write_hex64("VM kernel virtual base: 0x", plan->kernel_virtual_base);
    x86_64_serial_write_hex64("VM kernel virtual start: 0x", plan->kernel_virtual_start);
    x86_64_serial_write_hex64("VM kernel virtual end: 0x", plan->kernel_virtual_end);
    x86_64_serial_write_hex64("VM kernel virtual offset: 0x", plan->kernel_virtual_offset);
    x86_64_serial_write_hex64("VM direct map base: 0x", plan->direct_map_base);
    x86_64_serial_write_hex64("VM direct map end: 0x", plan->direct_map_end);
    x86_64_serial_write_hex64("VM direct map bytes: 0x", plan->direct_map_bytes);
    x86_64_serial_write_u32("VM identity PML4 index: ", plan->identity_pml4_index);
    x86_64_serial_write_u32("VM kernel PML4 index: ", plan->kernel_pml4_index);
    x86_64_serial_write_u32("VM direct map PML4 index: ", plan->direct_map_pml4_index);
    x86_64_serial_write_u32("VM planned regions: ", plan->planned_region_count);
    x86_64_serial_write_u32("VM identity window ok: ", plan->identity_window_ok);
    x86_64_serial_write_u32("VM kernel canonical: ", plan->kernel_window_canonical);
    x86_64_serial_write_u32("VM direct map canonical: ", plan->direct_map_canonical);
    x86_64_serial_write_u32("VM PML4 slots distinct: ", plan->pml4_slots_distinct);
    x86_64_serial_write_u32("VM plan ok: ", plan->plan_ok);
}

static void report_paging_builder(const struct x86_64_paging_builder_state *state)
{
    x86_64_serial_write_line("x86_64 paging builder online");
    x86_64_serial_write_hex64("Paging builder PML4: 0x", state->pml4_table);
    x86_64_serial_write_hex64("Paging builder identity PDPT: 0x", state->identity_pdpt_table);
    x86_64_serial_write_hex64("Paging builder identity PD: 0x", state->identity_pd_table);
    x86_64_serial_write_hex64("Paging builder direct PDPT: 0x", state->direct_pdpt_table);
    x86_64_serial_write_hex64("Paging builder direct PD: 0x", state->direct_pd_table);
    x86_64_serial_write_hex64("Paging builder framebuffer PDPT: 0x", state->framebuffer_pdpt_table);
    x86_64_serial_write_hex64("Paging builder framebuffer PD: 0x", state->framebuffer_pd_table);
    x86_64_serial_write_hex64("Paging builder kernel PDPT: 0x", state->kernel_pdpt_table);
    x86_64_serial_write_hex64("Paging builder kernel PD: 0x", state->kernel_pd_table);
    x86_64_serial_write_hex64("Paging builder kernel PT: 0x", state->kernel_pt_table);
    x86_64_serial_write_u32("Paging builder PMM backed: ", state->pmm_backed);
    x86_64_serial_write_u32("Paging builder table pages: ", state->allocated_table_pages);
    x86_64_serial_write_u32("Paging builder PMM free before: ", state->pmm_free_pages_before);
    x86_64_serial_write_u32("Paging builder PMM free after: ", state->pmm_free_pages_after);
    x86_64_serial_write_u32("Paging builder allocation ok: ", state->allocation_ok);
    x86_64_serial_write_u32("Paging builder PML4 entries: ", state->pml4_present_entries);
    x86_64_serial_write_u32("Paging builder identity huge pages: ", state->identity_huge_pages);
    x86_64_serial_write_u32("Paging builder direct huge pages: ", state->direct_huge_pages);
    x86_64_serial_write_hex64("Paging builder framebuffer physical: 0x", state->framebuffer_physical_base);
    x86_64_serial_write_hex64("Paging builder framebuffer virtual: 0x", state->framebuffer_virtual_address);
    x86_64_serial_write_hex64("Paging builder framebuffer bytes: 0x", state->framebuffer_bytes);
    x86_64_serial_write_u32("Paging builder framebuffer huge pages: ", state->framebuffer_huge_pages);
    x86_64_serial_write_u32("Paging builder kernel pages: ", state->kernel_pages);
    x86_64_serial_write_u32("Paging builder identity ok: ", state->identity_entry_ok);
    x86_64_serial_write_u32("Paging builder direct map ok: ", state->direct_map_entry_ok);
    x86_64_serial_write_u32("Paging builder framebuffer requested: ", state->framebuffer_requested);
    x86_64_serial_write_u32("Paging builder framebuffer entry ok: ", state->framebuffer_entry_ok);
    x86_64_serial_write_u32("Paging builder framebuffer mapped: ", state->framebuffer_mapped);
    x86_64_serial_write_u32("Paging builder kernel ok: ", state->kernel_entry_ok);
    x86_64_serial_write_u32("Paging builder tables aligned: ", state->tables_aligned);
    x86_64_serial_write_u32("Paging builder ok: ", state->builder_ok);
}

static void report_paging_activation(const struct x86_64_paging_activation_state *state)
{
    x86_64_serial_write_line("x86_64 candidate paging active");
    x86_64_serial_write_hex64("Paging activation previous CR3: 0x", state->previous_cr3);
    x86_64_serial_write_hex64("Paging activation requested CR3: 0x", state->requested_cr3);
    x86_64_serial_write_hex64("Paging activation active CR3: 0x", state->active_cr3);
    x86_64_serial_write_u32("Paging activation builder ready: ", state->builder_ready);
    x86_64_serial_write_u32("Paging activation CR3 changed: ", state->cr3_changed);
    x86_64_serial_write_u32("Paging activation active matches builder: ", state->active_matches_builder);
    x86_64_serial_write_u32("Paging activation ok: ", state->activation_ok);
}

static void report_paging_probes(const struct x86_64_paging_probe_state *state)
{
    x86_64_serial_write_line("x86_64 paging alias probes online");
    x86_64_serial_write_hex64("Paging probe identity addr: 0x", state->identity_addr);
    x86_64_serial_write_hex64("Paging probe direct map addr: 0x", state->direct_map_addr);
    x86_64_serial_write_hex64("Paging probe kernel alias addr: 0x", state->kernel_alias_addr);
    x86_64_serial_write_hex32("Paging probe identity value: 0x", state->identity_value);
    x86_64_serial_write_hex32("Paging probe direct map value: 0x", state->direct_map_value);
    x86_64_serial_write_hex32("Paging probe kernel alias value: 0x", state->kernel_alias_value);
    x86_64_serial_write_u32("Paging probe activation ready: ", state->activation_ready);
    x86_64_serial_write_u32("Paging probe identity ok: ", state->identity_probe_ok);
    x86_64_serial_write_u32("Paging probe direct map ok: ", state->direct_map_probe_ok);
    x86_64_serial_write_u32("Paging probe kernel alias ok: ", state->kernel_alias_probe_ok);
    x86_64_serial_write_u32("Paging probes ok: ", state->probes_ok);
}

static void report_higher_half_probe(const struct x86_64_higher_half_probe_state *state)
{
    x86_64_serial_write_line("x86_64 higher-half execution probe online");
    x86_64_serial_write_hex64("Higher-half probe low entry: 0x", state->low_entry);
    x86_64_serial_write_hex64("Higher-half probe high entry: 0x", state->high_entry);
    x86_64_serial_write_hex64("Higher-half probe bytes: 0x", state->entry_bytes);
    x86_64_serial_write_hex32("Higher-half probe expected: 0x", state->expected_value);
    x86_64_serial_write_hex32("Higher-half probe low result: 0x", state->low_result);
    x86_64_serial_write_hex32("Higher-half probe high result: 0x", state->high_result);
    x86_64_serial_write_u32("Higher-half probe activation ready: ", state->activation_ready);
    x86_64_serial_write_u32("Higher-half probe alias ready: ", state->alias_ready);
    x86_64_serial_write_u32("Higher-half probe low ok: ", state->low_probe_ok);
    x86_64_serial_write_u32("Higher-half probe high ok: ", state->high_probe_ok);
    x86_64_serial_write_u32("Higher-half probe ok: ", state->probe_ok);
    x86_64_serial_write_line("x86_64 higher-half C probe online");
    x86_64_serial_write_hex64("Higher-half C probe low entry: 0x", state->c_low_entry);
    x86_64_serial_write_hex64("Higher-half C probe high entry: 0x", state->c_high_entry);
    x86_64_serial_write_hex64("Higher-half C probe marker low: 0x", state->c_marker_low);
    x86_64_serial_write_hex64("Higher-half C probe marker high: 0x", state->c_marker_high);
    x86_64_serial_write_hex32("Higher-half C probe expected: 0x", state->c_expected_value);
    x86_64_serial_write_hex32("Higher-half C probe low result: 0x", state->c_low_result);
    x86_64_serial_write_hex32("Higher-half C probe high result: 0x", state->c_high_result);
    x86_64_serial_write_u32("Higher-half C probe alias ready: ", state->c_alias_ready);
    x86_64_serial_write_u32("Higher-half C probe low ok: ", state->c_low_probe_ok);
    x86_64_serial_write_u32("Higher-half C probe high ok: ", state->c_high_probe_ok);
    x86_64_serial_write_u32("Higher-half C probe ok: ", state->c_probe_ok);
    x86_64_serial_write_line("x86_64 higher-half handoff probe online");
    x86_64_serial_write_hex64("Higher-half handoff low entry: 0x", state->handoff_low_entry);
    x86_64_serial_write_hex64("Higher-half handoff high entry: 0x", state->handoff_high_entry);
    x86_64_serial_write_hex64("Higher-half handoff scratch low: 0x", state->handoff_scratch_low);
    x86_64_serial_write_hex64("Higher-half handoff arg: 0x", state->handoff_arg);
    x86_64_serial_write_hex32("Higher-half handoff expected: 0x", state->handoff_expected_value);
    x86_64_serial_write_hex32("Higher-half handoff result: 0x", state->handoff_result);
    x86_64_serial_write_hex32("Higher-half handoff scratch result: 0x", state->handoff_scratch_result);
    x86_64_serial_write_u32("Higher-half handoff ready: ", state->handoff_ready);
    x86_64_serial_write_u32("Higher-half handoff result ok: ", state->handoff_result_ok);
    x86_64_serial_write_u32("Higher-half handoff scratch ok: ", state->handoff_scratch_ok);
    x86_64_serial_write_u32("Higher-half handoff ok: ", state->handoff_ok);
}

static void report_runtime_entry(const struct x86_64_runtime_entry_state *state)
{
    x86_64_serial_write_line("x86_64 higher-half runtime entry online");
    x86_64_serial_write_hex64("Runtime low entry: 0x", state->low_entry);
    x86_64_serial_write_hex64("Runtime high entry: 0x", state->high_entry);
    x86_64_serial_write_hex64("Runtime state pointer: 0x", state->state_pointer);
    x86_64_serial_write_hex64("Runtime expected CR3: 0x", state->expected_cr3);
    x86_64_serial_write_hex64("Runtime active CR3: 0x", state->active_cr3);
    x86_64_serial_write_hex64("Runtime arg value: 0x", state->arg_value);
    x86_64_serial_write_hex64("Runtime stack sample: 0x", state->stack_sample);
    x86_64_serial_write_hex32("Runtime expected value: 0x", state->expected_value);
    x86_64_serial_write_hex32("Runtime return value: 0x", state->return_value);
    x86_64_serial_write_hex32("Runtime entered value: 0x", state->entered_value);
    x86_64_serial_write_hex32("Runtime scratch value: 0x", state->scratch_value);
    x86_64_serial_write_u32("Runtime activation ready: ", state->activation_ready);
    x86_64_serial_write_u32("Runtime high entry ready: ", state->high_entry_ready);
    x86_64_serial_write_u32("Runtime arg ok: ", state->arg_ok);
    x86_64_serial_write_u32("Runtime CR3 ok: ", state->cr3_ok);
    x86_64_serial_write_u32("Runtime return ok: ", state->return_ok);
    x86_64_serial_write_u32("Runtime entered ok: ", state->entered_ok);
    x86_64_serial_write_u32("Runtime scratch ok: ", state->scratch_ok);
    x86_64_serial_write_u32("Runtime stack identity ok: ", state->stack_identity_ok);
    x86_64_serial_write_u32("Runtime entry ok: ", state->runtime_ok);
}

static void report_user_mode(const struct x86_64_user_mode_state *state)
{
    x86_64_user_mode_report(state);
}

static void report_heap_state(const struct x86_64_heap_state *state)
{
    x86_64_console_write_u32(24, 0, "Heap smoke ok: ", state->smoke_ok);

    x86_64_serial_write_line("x86_64 early heap online");
    x86_64_serial_write_u32("Heap initialized: ", state->initialized);
    x86_64_serial_write_u32("Heap PMM backed: ", state->pmm_backed);
    x86_64_serial_write_u32("Heap direct map ok: ", state->direct_map_ok);
    x86_64_serial_write_u32("Heap pages: ", state->heap_pages);
    x86_64_serial_write_hex64("Heap bytes: 0x", state->heap_bytes);
    x86_64_serial_write_hex64("Heap used bytes: 0x", state->used_bytes);
    x86_64_serial_write_u32("Heap allocations: ", state->allocations);
    x86_64_serial_write_u32("Heap PMM free before: ", state->pmm_free_pages_before);
    x86_64_serial_write_u32("Heap PMM free after: ", state->pmm_free_pages_after);
    x86_64_serial_write_hex64("Heap first physical page: 0x", state->first_physical_page);
    x86_64_serial_write_hex64("Heap last physical page: 0x", state->last_physical_page);
    x86_64_serial_write_hex64("Heap first virtual page: 0x", state->first_virtual_page);
    x86_64_serial_write_hex64("Heap last virtual page: 0x", state->last_virtual_page);
    x86_64_serial_write_hex64("Heap first allocation: 0x", state->first_allocation);
    x86_64_serial_write_hex64("Heap second allocation: 0x", state->second_allocation);
    x86_64_serial_write_u32("Heap alignment ok: ", state->alignment_ok);
    x86_64_serial_write_u32("Heap pattern ok: ", state->pattern_ok);
    x86_64_serial_write_u32("Heap smoke ok: ", state->smoke_ok);
}

static void report_gdt_state(const struct x86_64_gdt_state *state)
{
    x86_64_serial_write_line("x86_64 maintained GDT online");
    x86_64_serial_write_hex64("GDT base: 0x", state->gdtr_base);
    x86_64_serial_write_u32("GDT limit: ", state->gdtr_limit);
    x86_64_serial_write_u32("GDT entries: ", state->entry_count);
    x86_64_serial_write_u32("GDT limit ok: ", state->limit_ok);
    x86_64_serial_write_u32("GDT selectors ok: ", state->selectors_ok);
    x86_64_serial_write_u32("GDT current CS: ", state->current_cs);
    x86_64_serial_write_u32("GDT current DS: ", state->current_ds);
    x86_64_serial_write_u32("GDT current SS: ", state->current_ss);
    x86_64_serial_write_u32("GDT current TR: ", state->current_tr);
    x86_64_serial_write_u32("GDT TSS loaded: ", state->tss_loaded);
    x86_64_serial_write_hex64("GDT null descriptor: 0x", state->null_descriptor);
    x86_64_serial_write_hex64("GDT code descriptor: 0x", state->code_descriptor);
    x86_64_serial_write_hex64("GDT data descriptor: 0x", state->data_descriptor);
    x86_64_serial_write_hex64("GDT TSS low descriptor: 0x", state->tss_descriptor_low);
    x86_64_serial_write_hex64("GDT TSS high descriptor: 0x", state->tss_descriptor_high);
}

static void report_tss_state(const struct x86_64_tss_state *state, const struct x86_64_gdt_state *gdt_state)
{
    u32 limit_ok = (state->tss_limit == X86_64_TSS_EXPECTED_LIMIT) ? 1U : 0U;
    u32 loaded_ok = ((gdt_state->selectors_ok != 0U) && (gdt_state->tss_loaded != 0U) &&
                     (state->initialized != 0U) && (limit_ok != 0U) &&
                     (state->loaded != 0U)) ? 1U : 0U;

    x86_64_serial_write_line("x86_64 bootstrap TSS loaded");
    x86_64_serial_write_hex64("TSS base: 0x", state->tss_base);
    x86_64_serial_write_u32("TSS limit: ", state->tss_limit);
    x86_64_serial_write_u32("TSS limit ok: ", limit_ok);
    x86_64_serial_write_hex64("TSS RSP0: 0x", state->rsp0);
    x86_64_serial_write_hex64("TSS IST1: 0x", state->ist1);
    x86_64_serial_write_hex64("TSS IST2: 0x", state->ist2);
    x86_64_serial_write_hex64("TSS IST3: 0x", state->ist3);
    x86_64_serial_write_hex64("TSS stack bytes: 0x", state->ist_stack_bytes);
    x86_64_serial_write_u32("TSS initialized: ", state->initialized);
    x86_64_serial_write_u32("TSS loaded: ", state->loaded);
    x86_64_serial_write_u32("GDT/TSS loaded ok: ", loaded_ok);
}

static void report_descriptor_summary(const struct x86_64_idt_state *idt_state,
                                      const struct x86_64_gdt_state *gdt_state,
                                      const struct x86_64_tss_state *tss_state)
{
    u32 limit_ok = (tss_state->tss_limit == X86_64_TSS_EXPECTED_LIMIT) ? 1U : 0U;
    u32 descriptor_ok = ((idt_state->ist_gates_ok != 0U) &&
                         (gdt_state->selectors_ok != 0U) &&
                         (gdt_state->tss_loaded != 0U) &&
                         (tss_state->loaded != 0U) &&
                         (limit_ok != 0U)) ? 1U : 0U;

    x86_64_console_write_u32(25, 0, "Desc/IST ok: ", descriptor_ok);
    x86_64_serial_write_u32("Desc/IST ok: ", descriptor_ok);
}

void kernel_main_x86_64(u32 boot_magic, u32 boot_info)
{
    struct x86_64_boot_context context;
    struct x86_64_cpuid_state cpuid_state;
    struct x86_64_desktop_services_state desktop_services_state;
    struct x86_64_framebuffer_state framebuffer_state;
    struct x86_64_gdt_state gdt_state;
    struct x86_64_heap_state heap_state;
    struct x86_64_higher_half_probe_state higher_half_probe;
    struct x86_64_idt_state idt_state;
    struct x86_64_paging_state paging_state;
    struct x86_64_paging_plan paging_plan;
    struct x86_64_paging_builder_state paging_builder;
    struct x86_64_paging_activation_state paging_activation;
    struct x86_64_paging_probe_state paging_probe;
    struct x86_64_pci_state pci_state;
    struct x86_64_runtime_entry_state runtime_entry;
    struct x86_64_storage_hw_state storage_hw_state;
    struct x86_64_tss_state tss_state;
    struct x86_64_user_mode_state user_mode_state;

    x86_64_console_init();
    x86_64_serial_init();

    x86_64_boot_context_init(boot_magic, boot_info, &context);
    x86_64_cpuid_state_init(&cpuid_state);
    x86_64_pci_scan(&pci_state);
    x86_64_storage_hw_init(&storage_hw_state, &pci_state);
    x86_64_paging_state_init(&paging_state);
    x86_64_pmm_init(&context.boot_info, &context.memory_layout);
    x86_64_paging_plan_init(&paging_plan, &context.memory_layout, &context.pmm_plan);
    x86_64_paging_builder_init(&paging_builder, &context.memory_layout, &paging_plan, &context.boot_info);
    x86_64_paging_builder_activate(&paging_activation, &paging_builder);
    x86_64_framebuffer_init(&framebuffer_state, &context.boot_info, &paging_builder);
    x86_64_framebuffer_run_smoke(&framebuffer_state);
    x86_64_heap_init(&heap_state, &paging_plan);
    x86_64_heap_run_smoke(&heap_state);
    x86_64_tss_init(&tss_state);
    x86_64_gdt_load_tss(&tss_state, &gdt_state);
    tss_state.loaded = gdt_state.tss_loaded;
    x86_64_idt_init();
    x86_64_idt_get_state(&idt_state);
    x86_64_desktop_services_init(&framebuffer_state);
    x86_64_desktop_services_run_smoke(&desktop_services_state);
    x86_64_paging_probe_active_mappings(&paging_probe, &paging_activation,
                                        &context.memory_layout, &paging_plan);
    x86_64_higher_half_probe_run(&higher_half_probe, &paging_activation, &paging_probe,
                                 &context.memory_layout, &paging_plan);
    x86_64_runtime_enter_higher_half(&runtime_entry, &paging_activation,
                                     &context.memory_layout, &paging_plan);
    report_framebuffer_summary(&context.boot_info);
    report_framebuffer_mapping_summary(&paging_builder);
    report_framebuffer_surface(&framebuffer_state);
    report_desktop_services(&desktop_services_state);
    report_pci_state(&pci_state);
    report_storage_hw_state(&storage_hw_state);
    x86_64_user_mode_start_init(&user_mode_state, &paging_builder, 1U);

    x86_64_console_write_at("Liam_OS x86_64 kernel diagnostics", 0, 0);
    x86_64_console_write_at("Stage: x86_64 ring3 smoke", 1, 0);

    x86_64_serial_write_line("Liam_OS x86_64 kernel diagnostics");
    x86_64_serial_write_line("Stage: x86_64 ring3 smoke");

    report_boot_summary(&context.boot_info);
    report_cpu_state(&cpuid_state);
    report_idt_state(&idt_state);
    report_memory_layout(&context.memory_layout);
    report_pmm_plan(&context.pmm_plan);
    report_pmm_allocator();
    report_paging_state(&paging_state);
    report_paging_plan(&paging_plan);
    report_paging_builder(&paging_builder);
    report_paging_activation(&paging_activation);
    report_paging_probes(&paging_probe);
    report_higher_half_probe(&higher_half_probe);
    report_runtime_entry(&runtime_entry);
    report_user_mode(&user_mode_state);
    report_heap_state(&heap_state);
    report_gdt_state(&gdt_state);
    report_tss_state(&tss_state, &gdt_state);
    report_descriptor_summary(&idt_state, &gdt_state, &tss_state);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
