#include "boot_context.h"
#include "console.h"
#include "cpuid.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "paging_builder.h"
#include "paging_plan.h"
#include "pmm.h"
#include "tss.h"

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
    x86_64_serial_write_hex64("Paging builder kernel PDPT: 0x", state->kernel_pdpt_table);
    x86_64_serial_write_hex64("Paging builder kernel PD: 0x", state->kernel_pd_table);
    x86_64_serial_write_hex64("Paging builder kernel PT: 0x", state->kernel_pt_table);
    x86_64_serial_write_u32("Paging builder PML4 entries: ", state->pml4_present_entries);
    x86_64_serial_write_u32("Paging builder identity huge pages: ", state->identity_huge_pages);
    x86_64_serial_write_u32("Paging builder direct huge pages: ", state->direct_huge_pages);
    x86_64_serial_write_u32("Paging builder kernel pages: ", state->kernel_pages);
    x86_64_serial_write_u32("Paging builder identity ok: ", state->identity_entry_ok);
    x86_64_serial_write_u32("Paging builder direct map ok: ", state->direct_map_entry_ok);
    x86_64_serial_write_u32("Paging builder kernel ok: ", state->kernel_entry_ok);
    x86_64_serial_write_u32("Paging builder tables aligned: ", state->tables_aligned);
    x86_64_serial_write_u32("Paging builder ok: ", state->builder_ok);
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

    x86_64_console_write_u32(24, 0, "Desc/IST ok: ", descriptor_ok);
    x86_64_serial_write_u32("Desc/IST ok: ", descriptor_ok);
}

void kernel_main_x86_64(u32 boot_magic, u32 boot_info)
{
    struct x86_64_boot_context context;
    struct x86_64_cpuid_state cpuid_state;
    struct x86_64_gdt_state gdt_state;
    struct x86_64_idt_state idt_state;
    struct x86_64_paging_state paging_state;
    struct x86_64_paging_plan paging_plan;
    struct x86_64_paging_builder_state paging_builder;
    struct x86_64_tss_state tss_state;

    x86_64_console_init();
    x86_64_serial_init();

    x86_64_boot_context_init(boot_magic, boot_info, &context);
    x86_64_cpuid_state_init(&cpuid_state);
    x86_64_paging_state_init(&paging_state);
    x86_64_paging_plan_init(&paging_plan, &context.memory_layout, &context.pmm_plan);
    x86_64_paging_builder_init(&paging_builder, &context.memory_layout, &paging_plan);
    x86_64_tss_init(&tss_state);
    x86_64_gdt_load_tss(&tss_state, &gdt_state);
    tss_state.loaded = gdt_state.tss_loaded;
    x86_64_idt_init();
    x86_64_idt_get_state(&idt_state);
    x86_64_pmm_init(&context.boot_info, &context.memory_layout);

    x86_64_console_write_at("Liam_OS x86_64 kernel diagnostics", 0, 0);
    x86_64_console_write_at("Stage: paging builder + descriptor", 1, 0);

    x86_64_serial_write_line("Liam_OS x86_64 kernel diagnostics");
    x86_64_serial_write_line("Stage: paging builder + descriptor");

    report_boot_summary(&context.boot_info);
    report_cpu_state(&cpuid_state);
    report_idt_state(&idt_state);
    report_memory_layout(&context.memory_layout);
    report_pmm_plan(&context.pmm_plan);
    report_pmm_allocator();
    report_paging_state(&paging_state);
    report_paging_plan(&paging_plan);
    report_paging_builder(&paging_builder);
    report_gdt_state(&gdt_state);
    report_tss_state(&tss_state, &gdt_state);
    report_descriptor_summary(&idt_state, &gdt_state, &tss_state);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
