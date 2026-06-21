#include "../drivers/vga.h"
#include "../drivers/timer.h"
#include "../drivers/keyboard.h"
#include "../arch/i386/gdt.h"
#include "../arch/i386/tss.h"
#include "../arch/i386/ring3.h"
#include "../arch/i386/context.h"
#include "../arch/i386/idt.h"
#include "../arch/i386/pic.h"
#include "../arch/i386/paging.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"
#include "../memory/vmm.h"
#include "shell.h"
#include "boot.h"
#include "log.h"
#include "panic.h"
#include "assert.h"
#include "system.h"
#include "version.h"
#include "scheduler.h"
#include "syscall.h"
#include "process.h"
#include "fd_table.h"
#include "user_image.h"
#include "exec.h"
#include "../fs/vfs.h"
#include "../fs/initramfs.h"
#include "../loader/flat_binary.h"
#include "../loader/elf32.h"

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_address) {
    boot_initialize(multiboot_magic, multiboot_info_address);

    vga_initialize();

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_line(LIAM_OS_NAME);
    vga_write_line("");

    log_success("Kernel started");
    log_success("VGA console initialized");
    kernel_assert_initialize();

    gdt_initialize();
    tss_set_kernel_stack(context_read_esp());
    idt_initialize();
    pic_remap();

    system_print_boot_info();

    if (!boot_get_info()->is_valid_multiboot) {
        kernel_panic("Bootloader did not provide valid Multiboot data");
    }

    boot_print_info();

    if (!boot_get_info()->has_memory_map) {
        kernel_panic("Bootloader did not provide a memory map");
    }

    pmm_initialize(boot_get_info());
    pmm_print_stats();

    paging_initialize();
    vmm_initialize();

    process_initialize();
    fd_table_initialize();
    syscall_initialize();
    ring3_initialize();

    vfs_initialize();
    initramfs_initialize();

    kernel_status_t initramfs_status = initramfs_mount();
    if (initramfs_status != KERNEL_OK) {
        kernel_panic("Unable to mount initramfs");
    }

    flat_binary_loader_initialize();
    elf32_initialize();
    user_image_initialize();
    exec_initialize();

    heap_initialize();

    timer_initialize(100);
    scheduler_initialize();
    keyboard_initialize();

    pic_clear_masks();

    log_success("Interrupts enabled");
    __asm__ volatile("sti");

    timer_wait_ticks(10);
    log_info_u32("Timer ticks after wait", timer_get_ticks());

    log_info("Starting initial userspace process");
    kernel_status_t init_status = exec_start_init();

    if (init_status == KERNEL_OK) {
        log_success("Initial userspace process exited cleanly");
    } else {
        log_warning("Initial userspace process returned an error");
        log_info_u32("Init status", (uint32_t)init_status);
    }

    /*
     * Uncomment this line only when testing the PMM allocator.
     */
    /* pmm_run_self_test(); */

    /*
     * Uncomment this line only when testing the IDT exception handler.
     */
    /* __asm__ volatile("int $3"); */

    vga_write_line("");
    log_success("Boot sequence complete");
    log_success("Status: interactive halt loop");

    shell_initialize();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}