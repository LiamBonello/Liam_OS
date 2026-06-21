#include "boot_context.h"
#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
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

static void report_memory_layout(const struct x86_64_memory_layout *layout)
{
    x86_64_console_write_at("IDT: exceptions installed", 10, 0);
    x86_64_console_write_hex64(11, 0, "Kernel start: 0x", layout->kernel_start);
    x86_64_console_write_hex64(12, 0, "Kernel end: 0x", layout->kernel_end);
    x86_64_console_write_hex64(13, 0, "Kernel bytes: 0x", layout->kernel_size_bytes);
    x86_64_console_write_hex64(14, 0, "Identity map bytes: 0x", layout->bootstrap_identity_map_bytes);

    x86_64_serial_write_line("x86_64 IDT: exceptions installed");
    x86_64_serial_write_hex64("Kernel start: 0x", layout->kernel_start);
    x86_64_serial_write_hex64("Kernel end: 0x", layout->kernel_end);
    x86_64_serial_write_hex64("Kernel bytes: 0x", layout->kernel_size_bytes);
    x86_64_serial_write_hex64("Identity map bytes: 0x", layout->bootstrap_identity_map_bytes);
}

static void report_pmm_plan(const struct x86_64_pmm_plan *plan)
{
    x86_64_console_write_u32(15, 0, "PMM usable regions: ", plan->usable_region_count);
    x86_64_console_write_hex64(16, 0, "PMM first page: 0x", plan->first_free_page);
    x86_64_console_write_hex64(17, 0, "PMM pages: 0x", plan->managed_pages);
    x86_64_console_write_hex64(18, 0, "PMM bytes: 0x", plan->managed_bytes);
    x86_64_console_write_hex64(19, 0, "Reserved below: 0x", plan->reserved_below);

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

    x86_64_console_write_u32(20, 0, "PMM tracked pages: ", state->tracked_pages);
    x86_64_console_write_hex64(21, 0, "PMM smoke page: 0x", page);
    x86_64_console_write_u32(22, 0, "PMM smoke free: ", free_ok);

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
    x86_64_console_write_u32(23, 0, "Paging huge pages: ", state->huge_pages_present);

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

    x86_64_console_write_u32(24, 0, "GDT/TSS loaded ok: ", loaded_ok);

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
}

void kernel_main_x86_64(u32 boot_magic, u32 boot_info)
{
    struct x86_64_boot_context context;
    struct x86_64_gdt_state gdt_state;
    struct x86_64_paging_state paging_state;
    struct x86_64_tss_state tss_state;

    x86_64_console_init();
    x86_64_serial_init();
    x86_64_idt_init();

    x86_64_boot_context_init(boot_magic, boot_info, &context);
    x86_64_paging_state_init(&paging_state);
    x86_64_tss_init(&tss_state);
    x86_64_gdt_load_tss(&tss_state, &gdt_state);
    tss_state.loaded = gdt_state.tss_loaded;
    x86_64_pmm_init(&context.boot_info, &context.memory_layout);

    x86_64_console_write_at("Liam_OS x86_64 kernel diagnostics", 0, 0);
    x86_64_console_write_at("Stage: descriptor load + paging + PMM", 1, 0);

    x86_64_serial_write_line("Liam_OS x86_64 kernel diagnostics");
    x86_64_serial_write_line("Stage: descriptor load + paging + PMM");

    report_boot_summary(&context.boot_info);
    report_memory_layout(&context.memory_layout);
    report_pmm_plan(&context.pmm_plan);
    report_pmm_allocator();
    report_paging_state(&paging_state);
    report_gdt_state(&gdt_state);
    report_tss_state(&tss_state, &gdt_state);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
