#include "shell.h"
#include "string.h"
#include "version.h"
#include "panic.h"
#include "assert.h"
#include "status.h"
#include "boot.h"
#include "scheduler.h"
#include "syscall.h"
#include "process.h"
#include "user_image.h"
#include "exec.h"
#include "fd_table.h"
#include "../fs/vfs.h"
#include "../fs/initramfs.h"
#include "../loader/flat_binary.h"
#include "../loader/elf32.h"
#include "../arch/i386/io.h"
#include "../arch/i386/paging.h"
#include "../arch/i386/context.h"
#include "../arch/i386/tss.h"
#include "../arch/i386/ring3.h"
#include "../drivers/vga.h"
#include "../drivers/timer.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"
#include "../memory/vmm.h"
#include "../memory/memory_layout.h"

#define SHELL_INPUT_MAX 128
#define SHELL_COMMAND_MAX 32

#define SHELL_MAX_TRACKED_PMM_ALLOCATIONS 32
#define SHELL_MAX_TRACKED_HEAP_ALLOCATIONS 32

typedef struct {
    void* address;
    uint32_t pages;
    uint8_t active;
} shell_pmm_allocation_t;

typedef struct {
    void* address;
    uint32_t bytes;
    uint8_t active;
} shell_heap_allocation_t;

typedef struct {
    char name[SHELL_COMMAND_MAX];
    const char* args;
} shell_command_t;

static char input_buffer[SHELL_INPUT_MAX];
static unsigned int input_length = 0;
static void* vmm_test_physical_page = 0;
static uint8_t vmm_test_mapping_active = 0;
static uint32_t shell_created_task_counter = 0;
static uint32_t shell_yield_workload_counter = 0;
static shell_pmm_allocation_t pmm_allocations[SHELL_MAX_TRACKED_PMM_ALLOCATIONS];
static shell_heap_allocation_t heap_allocations[SHELL_MAX_TRACKED_HEAP_ALLOCATIONS];

static void shell_trigger_page_fault(void);
static void shell_print_status_code(kernel_status_t status);

static void shell_clear_input(void) {
    string_clear(input_buffer, SHELL_INPUT_MAX);
    input_length = 0;
}

static void shell_clear_tracking_tables(void) {
    for (uint32_t i = 0; i < SHELL_MAX_TRACKED_PMM_ALLOCATIONS; i++) {
        pmm_allocations[i].address = 0;
        pmm_allocations[i].pages = 0;
        pmm_allocations[i].active = 0;
    }

    for (uint32_t i = 0; i < SHELL_MAX_TRACKED_HEAP_ALLOCATIONS; i++) {
        heap_allocations[i].address = 0;
        heap_allocations[i].bytes = 0;
        heap_allocations[i].active = 0;
    }
}

static shell_command_t shell_parse_command(const char* input) {
    shell_command_t command;
    const char* cursor = input;

    string_clear(command.name, SHELL_COMMAND_MAX);
    command.args = "";

    string_trim_left(&cursor);

    uint32_t name_length = string_copy_until_space(command.name, cursor, SHELL_COMMAND_MAX);
    cursor += name_length;
    cursor += string_skip_spaces(cursor, 0);

    command.args = cursor;

    return command;
}

void shell_print_prompt(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write("> ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

static void shell_print_help(void) {
    vga_write_line("Available commands:");
    vga_write_line("");
    vga_write_line("System:");
    vga_write_line("  help             Show available commands");
    vga_write_line("  about            Show Liam OS summary");
    vga_write_line("  version          Show OS version");
    vga_write_line("  sysinfo          Show boot/runtime information");
    vga_write_line("  clear            Clear the screen");
    vga_write_line("  reboot           Reboot the machine");
    vga_write_line("  panic            Trigger test kernel panic");
    vga_write_line("");
    vga_write_line("Diagnostics:");
    vga_write_line("  mem              Show memory statistics");
    vga_write_line("  heap             Show kernel heap statistics");
    vga_write_line("  heap-audit       Check kernel heap consistency");
    vga_write_line("  heap-validate <addr> Validate heap allocation pointer");
    vga_write_line("  paging           Show paging information");
    vga_write_line("  vmm              Show virtual memory manager statistics");
    vga_write_line("  vmm-audit        Check VMM mapping consistency");
    vga_write_line("  vmm-validate-status <addr> Status VMM mapping");
    vga_write_line("  layout           Show kernel virtual memory layout");
    vga_write_line("  virt <address>   Inspect virtual address mapping");
    vga_write_line("  map-info <addr>  Alias for virt <address>");
    vga_write_line("  scheduler        Show scheduler statistics");
    vga_write_line("  tasks            List scheduler tasks");
    vga_write_line("  policy           Show scheduler policy summary");
    vga_write_line("  readyq           Show ready queue snapshot");
    vga_write_line("  scheduler-audit  Check scheduler consistency");
    vga_write_line("  scheduler-repair Repair safe scheduler issues");
    vga_write_line("  scheduler-lock-status Show scheduler lock state");
    vga_write_line("  scheduler-lock-test Run scheduler lock self-test");
    vga_write_line("  assert           Show assertion statistics");
    vga_write_line("  assert-pass      Run non-failing assertion check");
    vga_write_line("  assert-fail      Trigger controlled assertion panic");
    vga_write_line("  status           Show kernel status code table");
    vga_write_line("  status-is <code> Explain one status code");
    vga_write_line("  sched-workload-create <n> Create scheduler workload");
    vga_write_line("  sched-workload-stop Stop built-in scheduler workload");
    vga_write_line("  sched-workload-reset Reset workload counters");
    vga_write_line("  sched-workload-status Show workload counters");
    vga_write_line("  tss-status       Show task-state segment status");
    vga_write_line("  ring3-status     Show user-mode transition status");
    vga_write_line("  syscalls         Show syscall dispatcher statistics");
    vga_write_line("  fd-status        Show file descriptor subsystem status");
    vga_write_line("  images           List registered userspace images");
    vga_write_line("  exec-status      Show exec and image registry status");
    vga_write_line("  fs-status        Show VFS and initramfs status");
    vga_write_line("  loader-status    Show userspace image loader status");
    vga_write_line("  elf-status       Show ELF32 loader status");
    vga_write_line("  elf-check <path> Validate an ELF32 file");
    vga_write_line("  ls <path>        List directory entries");
    vga_write_line("  cat <path>       Print file contents");
    vga_write_line("  stat <path>      Show filesystem node metadata");
    vga_write_line("  ps               Show process table");
    vga_write_line("  proc-status      Show process subsystem status");
    vga_write_line("  proc-info <pid>  Show detailed process information");
    vga_write_line("  proc-clear-exited Clear exited/failed process slots");
    vga_write_line("  wait-last        Show last exited process result");
    vga_write_line("  run-ready        Run the next READY userspace process");
    vga_write_line("  run-pid <pid>    Run a READY userspace process by PID");
    vga_write_line("  exec <path>      Start a userspace image");
    vga_write_line("  exec /bin/hello  Run the flat hello userspace program");
    vga_write_line("  exec /bin/hello-elf Run the ELF32 hello userspace program");
    vga_write_line("  exec /bin/syscheck Run the syscall test userspace program");
    vga_write_line("  exec /bin/pid    Print current userspace PID");
    vga_write_line("  exec /bin/ticks  Print current timer ticks");
    vga_write_line("  exec /bin/about  Print Liam_OS Core information");
    vga_write_line("  exec /bin/os-release Read /etc/os-release from userspace");
    vga_write_line("  exec /bin/cat    Read default file from userspace");
    vga_write_line("  exec /bin/cat /etc/os-release Read selected file from userspace");
    vga_write_line("  init             Start /sbin/init");
    vga_write_line("  context          Show current CPU context");
    vga_write_line("  ctx-test         Alias for context");
    vga_write_line("  ctx-self-test    Run context linkage self-test");
    vga_write_line("  switch-test      Run controlled context switch test");
    vga_write_line("  preempt-status   Show timer scheduler test status");
    vga_write_line("  preempt-enable-test <n> Enable timer scheduler test");
    vga_write_line("  preempt-disable-test Disable timer scheduler test");
    vga_write_line("  preempt-reset    Reset timer scheduler test counters");
    vga_write_line("  switch-status    Show context switch test status");
    vga_write_line("  switch-reset     Reset context switch test state");
    vga_write_line("  task-create <n>  Create recurring no-op task");
    vga_write_line("  task-once        Create one-shot no-op task");
    vga_write_line("  task-info <id>   Show detailed task info");
    vga_write_line("  task-run-now <id> Run task immediately");
    vga_write_line("  task-switch-run <id> Run task on its kernel stack");
    vga_write_line("  task-sleep <id> <n> Sleep task for n ticks");
    vga_write_line("  task-wake <id>   Wake sleeping task");
    vga_write_line("  task-sleep-status <id> <n> Status sleep task");
    vga_write_line("  task-wake-status <id> Status wake task");
    vga_write_line("  task-disable-status <id> Status disable task");
    vga_write_line("  task-yield <id>  Defer task until next interval");
    vga_write_line("  task-current     Show current running task");
    vga_write_line("  task-disable <id> Disable task by id");
    vga_write_line("  task-clear       Clear completed/disabled tasks");
    vga_write_line("  pf-test          Trigger controlled page fault");
    vga_write_line("  ticks            Show raw timer ticks");
    vga_write_line("  uptime           Show uptime in seconds");
    vga_write_line("");
    vga_write_line("Utilities:");
    vga_write_line("  echo <text>      Print text");
    vga_write_line("  color <name>     Set shell text color");
    vga_write_line("");
    vga_write_line("PMM tests:");
    vga_write_line("  alloc <pages>    Allocate physical page(s)");
    vga_write_line("  pmm-list         List tracked PMM allocations");
    vga_write_line("  free-last        Free latest tracked PMM allocation");
    vga_write_line("  pmm-free-all     Free all tracked PMM allocations");
    vga_write_line("");
    vga_write_line("Heap tests:");
    vga_write_line("  kmalloc <bytes>  Allocate kernel heap memory");
    vga_write_line("  heap-list        List tracked heap allocations");
    vga_write_line("  kfree-last       Free latest tracked heap allocation");
    vga_write_line("  heap-free-all    Free all tracked heap allocations");
    vga_write_line("  heap-test        Run controlled heap stress test");
    vga_write_line("");
    vga_write_line("VMM tests:");
    vga_write_line("  vmap-test        Map and write virtual address 0x08000000");
    vga_write_line("  vunmap-test      Unmap virtual address 0x08000000");
}

static void shell_print_about(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_line("Liam OS");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_write_line("A from-scratch hobby operating system.");
    vga_write_line("Current stage: kernel console, IRQs, PMM, paging, heap, and shell.");
    vga_write_line("Long-term target: graphical desktop OS with apps and tools.");
}

static void shell_print_version(void) {
    vga_write("Name: ");
    vga_write_line(LIAM_OS_NAME);

    vga_write("Version: ");
    vga_write_line(LIAM_OS_VERSION);

    vga_write("Codename: ");
    vga_write_line(LIAM_OS_CODENAME);

    vga_write("Architecture: ");
    vga_write_line(LIAM_OS_ARCH);

    vga_write("Boot protocol: ");
    vga_write_line(LIAM_OS_BOOT_PROTOCOL);
}

static void shell_print_sysinfo(void) {
    const boot_info_t* boot_info = boot_get_info();

    vga_write_line("System information:");

    vga_write("  OS: ");
    vga_write(LIAM_OS_NAME);
    vga_write(" ");
    vga_write_line(LIAM_OS_VERSION);

    vga_write("  Codename: ");
    vga_write_line(LIAM_OS_CODENAME);

    vga_write("  Architecture: ");
    vga_write_line(LIAM_OS_ARCH);

    vga_write("  Bootloader: ");
    vga_write_line(LIAM_OS_BOOTLOADER);

    vga_write("  Multiboot valid: ");
    vga_write_line(boot_info->is_valid_multiboot ? "yes" : "no");

    vga_write("  Memory map: ");
    vga_write_line(boot_info->has_memory_map ? "yes" : "no");

    vga_write("  Memory map regions: ");
    vga_write_u32(boot_info->memory_map_region_count);
    vga_write_line("");

    vga_write("  Timer frequency Hz: ");
    vga_write_u32(timer_get_frequency_hz());
    vga_write_line("");

    vga_write("  Timer ticks: ");
    vga_write_u32(timer_get_ticks());
    vga_write_line("");
}


static void shell_print_tss_status(void) {
    const tss_info_t* info = tss_get_info();

    vga_write_line("Task State Segment status:");

    vga_write("  Initialized: ");
    vga_write_line(info->initialized ? "yes" : "no");

    vga_write("  Loaded: ");
    vga_write_line(info->loaded ? "yes" : "no");

    vga_write("  Selector: ");
    vga_write_hex_u32(info->selector);
    vga_write_line("");

    vga_write("  Kernel SS0: ");
    vga_write_hex_u32(info->ss0);
    vga_write_line("");

    vga_write("  Kernel ESP0: ");
    vga_write_hex_u32(info->esp0);
    vga_write_line("");

    vga_write("  TSS base: ");
    vga_write_hex_u32(info->base);
    vga_write_line("");

    vga_write("  TSS limit: ");
    vga_write_u32(info->limit);
    vga_write_line("");

    vga_write("  Stack updates: ");
    vga_write_u32(info->set_kernel_stack_count);
    vga_write_line("");
}

static void shell_print_ring3_status(void) {
    const ring3_info_t* info = ring3_get_info();

    vga_write_line("Ring 3 execution status:");

    vga_write("  Initialized: ");
    vga_write_line(info->initialized ? "yes" : "no");

    vga_write("  Ready: ");
    vga_write_line(info->ready ? "yes" : "no");

    vga_write("  In progress: ");
    vga_write_line(info->in_progress ? "yes" : "no");

    vga_write("  Completed: ");
    vga_write_line(info->completed ? "yes" : "no");

    vga_write("  Runs: ");
    vga_write_u32(info->runs);
    vga_write_line("");

    vga_write("  Successful runs: ");
    vga_write_u32(info->successful_runs);
    vga_write_line("");

    vga_write("  Failed runs: ");
    vga_write_u32(info->failed_runs);
    vga_write_line("");

    vga_write("  Syscalls: ");
    vga_write_u32(info->syscall_count);
    vga_write_line("");

    vga_write("  Image load attempts: ");
    vga_write_u32(info->load_attempts);
    vga_write_line("");

    vga_write("  Successful image loads: ");
    vga_write_u32(info->successful_loads);
    vga_write_line("");

    vga_write("  Failed image loads: ");
    vga_write_u32(info->failed_loads);
    vga_write_line("");

    vga_write("  Last syscall: ");
    vga_write_u32(info->last_syscall_number);
    vga_write_line("");

    vga_write("  User code virtual: ");
    vga_write_hex_u32(info->user_code_virtual);
    vga_write_line("");

    vga_write("  User stack virtual: ");
    vga_write_hex_u32(info->user_stack_virtual);
    vga_write_line("");

    vga_write("  User stack top: ");
    vga_write_hex_u32(info->user_stack_top);
    vga_write_line("");

    vga_write("  Code physical: ");
    vga_write_hex_u32(info->code_physical);
    vga_write_line("");

    vga_write("  Stack physical: ");
    vga_write_hex_u32(info->stack_physical);
    vga_write_line("");

    vga_write("  Kernel syscall stack top: ");
    vga_write_hex_u32(info->kernel_syscall_stack_top);
    vga_write_line("");

    vga_write("  Previous TSS ESP0: ");
    vga_write_hex_u32(info->previous_tss_esp0);
    vga_write_line("");

    vga_write("  Last status: ");
    shell_print_status_code(info->last_status);
}


static void shell_print_syscall_status(void) {
    const syscall_info_t* info = syscall_get_info();

    vga_write_line("Syscall status:");

    vga_write("  Total calls: ");
    vga_write_u32(info->total_calls);
    vga_write_line("");

    vga_write("  Last number: ");
    vga_write_u32(info->last_number);
    vga_write_line("");

    vga_write("  Last arg1: ");
    vga_write_hex_u32(info->last_arg1);
    vga_write_line("");

    vga_write("  Last arg2: ");
    vga_write_hex_u32(info->last_arg2);
    vga_write_line("");

    vga_write("  Last arg3: ");
    vga_write_hex_u32(info->last_arg3);
    vga_write_line("");

    vga_write("  Unsupported calls: ");
    vga_write_u32(info->unsupported_calls);
    vga_write_line("");
}

static void shell_print_fd_status(void)
{
    const fd_table_info_t* info = fd_table_get_info();

    vga_write_line("File descriptor subsystem:");

    vga_write("  Initialized: ");
    vga_write_line(info->initialized ? "yes" : "no");

    vga_write("  Open attempts: ");
    vga_write_u32(info->open_attempts);
    vga_write_line("");

    vga_write("  Successful opens: ");
    vga_write_u32(info->successful_opens);
    vga_write_line("");

    vga_write("  Failed opens: ");
    vga_write_u32(info->failed_opens);
    vga_write_line("");

    vga_write("  Read attempts: ");
    vga_write_u32(info->read_attempts);
    vga_write_line("");

    vga_write("  Successful reads: ");
    vga_write_u32(info->successful_reads);
    vga_write_line("");

    vga_write("  Failed reads: ");
    vga_write_u32(info->failed_reads);
    vga_write_line("");

    vga_write("  Close attempts: ");
    vga_write_u32(info->close_attempts);
    vga_write_line("");

    vga_write("  Successful closes: ");
    vga_write_u32(info->successful_closes);
    vga_write_line("");

    vga_write("  Failed closes: ");
    vga_write_u32(info->failed_closes);
    vga_write_line("");

    vga_write("  Last fd: ");
    if (info->last_fd == FD_INVALID)
    {
        vga_write_line("none");
    }
    else
    {
        vga_write_u32(info->last_fd);
        vga_write_line("");
    }

    vga_write("  Last path: ");
    vga_write_line(info->last_path);

    vga_write("  Last status: ");
    shell_print_status_code(info->last_status);
}

static void shell_print_loader_status(void)
{
    const flat_binary_loader_info_t* info = flat_binary_loader_get_info();

    vga_write_line("Flat binary loader:");

    vga_write("  Initialized: ");
    vga_write_line(info->initialized ? "yes" : "no");

    vga_write("  Load attempts: ");
    vga_write_u32(info->load_attempts);
    vga_write_line("");

    vga_write("  Successful loads: ");
    vga_write_u32(info->successful_loads);
    vga_write_line("");

    vga_write("  Failed loads: ");
    vga_write_u32(info->failed_loads);
    vga_write_line("");

    vga_write("  Last path: ");
    vga_write_line(info->last_path);

    vga_write("  Last image size: ");
    vga_write_u32(info->last_image_size);
    vga_write_line("");

    vga_write("  Last entry: ");
    vga_write_hex_u32(info->last_entry);
    vga_write_line("");

    vga_write("  Last stack top: ");
    vga_write_hex_u32(info->last_stack_top);
    vga_write_line("");

    vga_write("  Last status: ");
    shell_print_status_code(info->last_status);
}

static void shell_print_elf_status(void)
{
    const elf32_loader_info_t* info = elf32_get_info();

    vga_write_line("ELF32 loader:");

    vga_write("  Initialized: ");
    vga_write_line(info->initialized ? "yes" : "no");

    vga_write("  Validation attempts: ");
    vga_write_u32(info->validation_attempts);
    vga_write_line("");

    vga_write("  Successful validations: ");
    vga_write_u32(info->successful_validations);
    vga_write_line("");

    vga_write("  Failed validations: ");
    vga_write_u32(info->failed_validations);
    vga_write_line("");

    vga_write("  Load attempts: ");
    vga_write_u32(info->load_attempts);
    vga_write_line("");

    vga_write("  Successful loads: ");
    vga_write_u32(info->successful_loads);
    vga_write_line("");

    vga_write("  Failed loads: ");
    vga_write_u32(info->failed_loads);
    vga_write_line("");

    vga_write("  Last path: ");
    vga_write_line(info->last_path);

    vga_write("  Last file size: ");
    vga_write_u32(info->last_file_size);
    vga_write_line("");

    vga_write("  Last entry: ");
    vga_write_hex_u32(info->last_entry);
    vga_write_line("");

    vga_write("  Last program headers: ");
    vga_write_u32(info->last_program_header_count);
    vga_write_line("");

    vga_write("  Last load segment offset: ");
    vga_write_u32(info->last_load_segment_offset);
    vga_write_line("");

    vga_write("  Last load segment vaddr: ");
    vga_write_hex_u32(info->last_load_segment_vaddr);
    vga_write_line("");

    vga_write("  Last load segment file size: ");
    vga_write_u32(info->last_load_segment_file_size);
    vga_write_line("");

    vga_write("  Last load segment memory size: ");
    vga_write_u32(info->last_load_segment_memory_size);
    vga_write_line("");

    vga_write("  Last status: ");
    shell_print_status_code(info->last_status);
}

static void shell_handle_elf_check(const char* args)
{
    const char* path = args;
    string_trim_left(&path);

    if (path[0] == '\0')
    {
        vga_write_line("Usage: elf-check <absolute-path>");
        vga_write_line("Example: elf-check /tests/elf32-sample");
        return;
    }

    const vfs_node_t* node = 0;
    kernel_status_t status = vfs_lookup(path, &node);

    if (status != KERNEL_OK)
    {
        vga_write("elf-check lookup failed: ");
        shell_print_status_code(status);
        return;
    }

    const elf32_header_t* header = 0;
    status = elf32_validate_from_node(node, &header);

    if (status != KERNEL_OK)
    {
        vga_write("ELF32 validation failed: ");
        shell_print_status_code(status);
        return;
    }

    vga_write_line("ELF32 validation: OK");

    vga_write("  Path: ");
    vga_write_line(path);

    vga_write("  Entry: ");
    vga_write_hex_u32(header->entry);
    vga_write_line("");

    vga_write("  Program header offset: ");
    vga_write_u32(header->program_header_offset);
    vga_write_line("");

    vga_write("  Program header count: ");
    vga_write_u32(header->program_header_count);
    vga_write_line("");

    vga_write("  Section header count: ");
    vga_write_u32(header->section_header_count);
    vga_write_line("");
}

static void shell_print_user_images(void)
{
    vga_write_line("Registered userspace images:");

    uint32_t count = user_image_count();

    if (count == 0)
    {
        vga_write_line("  none");
        return;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        user_image_t image;
        kernel_status_t status = user_image_get_at(i, &image);

        if (status != KERNEL_OK)
        {
            vga_write("  [unavailable] status=");
            vga_write_u32((uint32_t)status);
            vga_write_line("");
            continue;
        }

        vga_write("  ");
        vga_write(image.path);
        vga_write(" name=");
        vga_write(image.name);
        vga_write(" kind=");
        vga_write(user_image_kind_name(image.kind));
        if (image.entry != 0)
        {
            vga_write(" entry=");
            vga_write_hex_u32(image.entry);
        }
        if (image.user_stack_top != 0)
        {
            vga_write(" stack=");
            vga_write_hex_u32(image.user_stack_top);
        }
        vga_write(" size=");
        vga_write_u32(image.image_size);
        vga_write_line("");
    }
}

static void shell_print_signed_i32(int32_t value)
{
    if (value < 0)
    {
        vga_write("-");
        vga_write_u32((uint32_t)(0 - value));
        return;
    }

    vga_write_u32((uint32_t)value);
}

static void shell_print_processes(void)
{
    const process_t* table = process_get_table();

    vga_write_line("Process table:");

    for (uint32_t i = 0; i < PROCESS_MAX_PROCESSES; i++)
    {
        if (table[i].state == PROCESS_UNUSED)
        {
            continue;
        }

        vga_write("  pid=");
        vga_write_u32(table[i].pid);

        vga_write(" name=");
        vga_write(table[i].name);

        vga_write(" state=");
        vga_write(process_state_name(table[i].state));

        vga_write(" runs=");
        vga_write_u32(table[i].runs);

        vga_write(" exit=");
        shell_print_signed_i32(table[i].exit_code);

        vga_write(" status=");
        shell_print_status_code(table[i].last_status);

        vga_write_line("");
    }
}

static void shell_print_process_status(void)
{
    const process_info_t* info = process_get_info();

    vga_write_line("Process subsystem:");

    vga_write("  Initialized: ");
    vga_write_line(info->initialized ? "yes" : "no");

    vga_write("  Table capacity: ");
    vga_write_u32(info->table_capacity);
    vga_write_line("");

    vga_write("  Used slots: ");
    vga_write_u32(info->used_slots);
    vga_write_line("");

    vga_write("  Current PID: ");
    vga_write_u32(info->current_pid);
    vga_write_line("");

    vga_write("  Next PID: ");
    vga_write_u32(info->next_pid);
    vga_write_line("");

    vga_write("  Create attempts: ");
    vga_write_u32(info->create_attempts);
    vga_write_line("");

    vga_write("  Successful creates: ");
    vga_write_u32(info->successful_creates);
    vga_write_line("");

    vga_write("  Failed creates: ");
    vga_write_u32(info->failed_creates);
    vga_write_line("");

    vga_write("  Run attempts: ");
    vga_write_u32(info->run_attempts);
    vga_write_line("");

    vga_write("  Successful runs: ");
    vga_write_u32(info->successful_runs);
    vga_write_line("");

    vga_write("  Failed runs: ");
    vga_write_u32(info->failed_runs);
    vga_write_line("");

    vga_write("  Exit count: ");
    vga_write_u32(info->exit_count);
    vga_write_line("");

    vga_write("  Failed processes: ");
    vga_write_u32(info->failed_processes);
    vga_write_line("");

    vga_write("  Last created PID: ");
    vga_write_u32(info->last_created_pid);
    vga_write_line("");

    vga_write("  Last run PID: ");
    vga_write_u32(info->last_run_pid);
    vga_write_line("");

    vga_write("  Last exit PID: ");
    vga_write_u32(info->last_exit_pid);
    vga_write_line("");

    vga_write("  Last exit code: ");
    shell_print_signed_i32(info->last_exit_code);
    vga_write_line("");

    vga_write("  Last status: ");
    shell_print_status_code(info->last_status);
}

static void shell_handle_proc_info(const char* args)
{
    const char* cursor = args;
    string_trim_left(&cursor);

    if (cursor[0] == '\0')
    {
        vga_write_line("Usage: proc-info <pid>");
        vga_write_line("Example: proc-info 1");
        return;
    }

    uint32_t pid = 0;

    if (!string_to_uint32_auto(cursor, &pid))
    {
        vga_write_line("Invalid PID");
        return;
    }

    const process_t* process = process_find_by_pid(pid);

    if (process == 0)
    {
        vga_write_line("Process not found");
        return;
    }

    vga_write_line("Process information:");

    vga_write("  PID: ");
    vga_write_u32(process->pid);
    vga_write_line("");

    vga_write("  Name: ");
    vga_write_line(process->name);

    vga_write("  State: ");
    vga_write_line(process_state_name(process->state));

    vga_write("  Entry: ");
    vga_write_hex_u32(process->entry);
    vga_write_line("");

    vga_write("  User stack: ");
    vga_write_hex_u32(process->user_stack);
    vga_write_line("");

    vga_write("  Runs: ");
    vga_write_u32(process->runs);
    vga_write_line("");

    vga_write("  Exit code: ");
    shell_print_signed_i32(process->exit_code);
    vga_write_line("");

    vga_write("  Last status: ");
    shell_print_status_code(process->last_status);
}

static void shell_handle_proc_clear_exited(void)
{
    uint32_t cleared = process_clear_exited();

    vga_write("Cleared process slots: ");
    vga_write_u32(cleared);
    vga_write_line("");
}

static void shell_handle_wait_last(void)
{
    const process_info_t* info = process_get_info();

    if (!info->has_last_exit)
    {
        vga_write_line("No exited process recorded.");
        return;
    }

    vga_write_line("Last exited process:");

    vga_write("  PID: ");
    vga_write_u32(info->last_exit_pid);
    vga_write_line("");

    vga_write("  Name: ");
    vga_write_line(info->last_exit_name);

    vga_write("  Exit code: ");
    shell_print_signed_i32(info->last_exit_code);
    vga_write_line("");

    vga_write("  Status: ");
    shell_print_status_code(info->last_exit_status);
}

static void shell_handle_run_ready(void)
{
    process_t* process = 0;
    kernel_status_t status = process_run_next_ready(&process);

    if (status == KERNEL_ERROR_NOT_FOUND)
    {
        vga_write_line("No READY process found.");
        return;
    }

    vga_write("run-ready result: ");
    shell_print_status_code(status);

    if (process != 0)
    {
        vga_write("Process: pid=");
        vga_write_u32(process->pid);
        vga_write(" name=");
        vga_write(process->name);
        vga_write(" state=");
        vga_write_line(process_state_name(process->state));
    }
}

static void shell_handle_run_pid(const char* args)
{
    const char* cursor = args;
    string_trim_left(&cursor);

    if (cursor[0] == '\0')
    {
        vga_write_line("Usage: run-pid <pid>");
        vga_write_line("Example: run-pid 3");
        return;
    }

    uint32_t pid = 0;

    if (!string_to_uint32_auto(cursor, &pid))
    {
        vga_write_line("Invalid PID");
        return;
    }

    process_t* process = 0;
    kernel_status_t status = process_run_by_pid(pid, &process);

    vga_write("run-pid result: ");
    shell_print_status_code(status);

    if (process != 0)
    {
        vga_write("Process: pid=");
        vga_write_u32(process->pid);
        vga_write(" name=");
        vga_write(process->name);
        vga_write(" state=");
        vga_write_line(process_state_name(process->state));
    }
}

static void shell_print_exec_status(void)
{
    const exec_info_t* info = exec_get_info();
    const user_image_info_t* image_info = user_image_get_info();

    vga_write_line("Exec subsystem:");

    vga_write("  Initialized: ");
    vga_write_line(info->initialized ? "yes" : "no");

    vga_write("  Spawn attempts: ");
    vga_write_u32(info->spawn_attempts);
    vga_write_line("");

    vga_write("  Successful spawns: ");
    vga_write_u32(info->successful_spawns);
    vga_write_line("");

    vga_write("  Failed spawns: ");
    vga_write_u32(info->failed_spawns);
    vga_write_line("");

    vga_write("  Last PID: ");
    vga_write_u32(info->last_pid);
    vga_write_line("");

    vga_write("  Last path: ");
    vga_write_line(info->last_path);

    vga_write("  Last status: ");
    shell_print_status_code(info->last_status);

    vga_write_line("User image registry:");

    vga_write("  Registered images: ");
    vga_write_u32(image_info->registered_images);
    vga_write_line("");

    vga_write("  Resolve attempts: ");
    vga_write_u32(image_info->resolve_attempts);
    vga_write_line("");

    vga_write("  Resolve failures: ");
    vga_write_u32(image_info->resolve_failures);
    vga_write_line("");

    vga_write("  Last path: ");
    vga_write_line(image_info->last_path);

    vga_write("  Last status: ");
    shell_print_status_code(image_info->last_status);
}

static void shell_handle_exec(const char* args)
{
    const char* path = args;
    string_trim_left(&path);

    if (path[0] == '\0')
    {
        vga_write_line("Usage: exec <absolute-path>");
        vga_write_line("Example: exec /bin/hello");
        vga_write_line("Example: exec /bin/hello-elf");
        vga_write_line("Example: exec /bin/syscheck");
        vga_write_line("Example: exec /bin/pid");
        vga_write_line("Example: exec /bin/ticks");
        vga_write_line("Example: exec /bin/about");
        vga_write_line("Example: exec /bin/os-release");
        vga_write_line("Example: exec /bin/cat");
        vga_write_line("Example: exec /bin/cat /etc/os-release");
        vga_write_line("Example: exec /sbin/init");
        return;
    }

    char exec_path[64];
    uint32_t path_length = string_copy_until_space(exec_path, path, sizeof(exec_path));

    if (path_length == 0)
    {
        vga_write_line("Usage: exec <absolute-path> [args]");
        return;
    }

    const char* exec_args = path + path_length;
    string_trim_left(&exec_args);

    vga_write("Starting userspace image: ");
    vga_write_line(exec_path);

    if (exec_args[0] != '\0')
    {
        vga_write("Arguments: ");
        vga_write_line(exec_args);
    }

    process_t* process = 0;
    kernel_status_t status = exec_spawn_with_args(exec_path, exec_args, &process);

    vga_write("Exec result: ");
    shell_print_status_code(status);

    if (status == KERNEL_OK && process != 0)
    {
        vga_write("Process exited: pid=");
        vga_write_u32(process->pid);
        vga_write(" code=");
        vga_write_u32((uint32_t)process->exit_code);
        vga_write_line("");
    }
}

static void shell_start_init(void)
{
    vga_write_line("Starting system init process: /sbin/init");

    kernel_status_t status = exec_start_init();

    vga_write("Init result: ");
    shell_print_status_code(status);
}


static void shell_print_fs_status(void)
{
    const vfs_info_t* info = vfs_get_info();
    const initramfs_info_t* ramfs = initramfs_get_info();

    vga_write_line("VFS subsystem:");

    vga_write("  Initialized: ");
    vga_write_line(info->initialized ? "yes" : "no");

    vga_write("  Mounted nodes: ");
    vga_write_u32(info->mounted_nodes);
    vga_write_line("");

    vga_write("  Lookup attempts: ");
    vga_write_u32(info->lookup_attempts);
    vga_write_line("");

    vga_write("  Lookup failures: ");
    vga_write_u32(info->lookup_failures);
    vga_write_line("");

    vga_write("  Read attempts: ");
    vga_write_u32(info->read_attempts);
    vga_write_line("");

    vga_write("  Read failures: ");
    vga_write_u32(info->read_failures);
    vga_write_line("");

    vga_write("  Last path: ");
    vga_write_line(info->last_path);

    vga_write("  Last status: ");
    shell_print_status_code(info->last_status);

    vga_write_line("Initramfs:");

    vga_write("  Initialized: ");
    vga_write_line(ramfs->initialized ? "yes" : "no");

    vga_write("  Node count: ");
    vga_write_u32(ramfs->node_count);
    vga_write_line("");

    vga_write("  Mount count: ");
    vga_write_u32(ramfs->mount_count);
    vga_write_line("");

    vga_write("  Last status: ");
    shell_print_status_code(ramfs->last_status);
}

static const char* shell_path_or_root(const char* args)
{
    const char* path = args;
    string_trim_left(&path);

    if (path[0] == '\0')
    {
        return "/";
    }

    return path;
}

static void shell_handle_ls(const char* args)
{
    const char* path = shell_path_or_root(args);
    const vfs_node_t* const* children = 0;
    uint32_t count = 0;

    kernel_status_t status = vfs_list_directory(path, &children, &count);

    if (status != KERNEL_OK)
    {
        vga_write("ls failed: ");
        shell_print_status_code(status);
        return;
    }

    vga_write("Directory: ");
    vga_write_line(path);

    if (count == 0)
    {
        vga_write_line("  empty");
        return;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        const vfs_node_t* node = children[i];

        if (node == 0)
        {
            continue;
        }

        vga_write("  ");
        vga_write(vfs_node_type_name(node->type));
        vga_write(" ");
        vga_write(node->name);
        vga_write(" size=");
        vga_write_u32(node->size);
        vga_write_line("");
    }
}

static void shell_handle_stat(const char* args)
{
    const char* path = args;
    string_trim_left(&path);

    if (path[0] == '\0')
    {
        vga_write_line("Usage: stat <absolute-path>");
        return;
    }

    const vfs_node_t* node = 0;
    kernel_status_t status = vfs_lookup(path, &node);

    if (status != KERNEL_OK)
    {
        vga_write("stat failed: ");
        shell_print_status_code(status);
        return;
    }

    vga_write_line("Filesystem node:");
    vga_write("  Path: ");
    vga_write_line(node->path);
    vga_write("  Name: ");
    vga_write_line(node->name);
    vga_write("  Type: ");
    vga_write_line(vfs_node_type_name(node->type));
    vga_write("  Size: ");
    vga_write_u32(node->size);
    vga_write_line("");
    vga_write("  Mode: ");
    vga_write_u32(node->mode);
    vga_write_line("");
    vga_write("  Children: ");
    vga_write_u32(node->child_count);
    vga_write_line("");
}

static void shell_handle_cat(const char* args)
{
    const char* path = args;
    string_trim_left(&path);

    if (path[0] == '\0')
    {
        vga_write_line("Usage: cat <absolute-path>");
        return;
    }

    const uint8_t* data = 0;
    uint32_t size = 0;
    kernel_status_t status = vfs_read_file(path, &data, &size);

    if (status != KERNEL_OK)
    {
        vga_write("cat failed: ");
        shell_print_status_code(status);
        return;
    }

    if (data == 0 || size == 0)
    {
        return;
    }

    for (uint32_t i = 0; i < size; i++)
    {
        vga_write_char((char)data[i]);
    }

    if (data[size - 1] != '\n')
    {
        vga_write_line("");
    }
}

static const char* shell_scheduler_state_name(scheduler_task_state_t state) {
    switch (state) {
    case SCHEDULER_TASK_UNUSED:
        return "unused";
    case SCHEDULER_TASK_ACTIVE:
        return "active";
    case SCHEDULER_TASK_RUNNING:
        return "running";
    case SCHEDULER_TASK_DISABLED:
        return "disabled";
    case SCHEDULER_TASK_COMPLETED:
        return "completed";
    case SCHEDULER_TASK_SLEEPING:
        return "sleeping";
    default:
        return "unknown";
    }
}

static const char* shell_scheduler_mode_name(scheduler_task_mode_t mode) {
    switch (mode) {
    case SCHEDULER_TASK_RECURRING:
        return "recurring";
    case SCHEDULER_TASK_ONESHOT:
        return "oneshot";
    default:
        return "unknown";
    }
}

static void shell_print_scheduler(void) {
    const scheduler_stats_t* stats = scheduler_get_stats();

    vga_write_line("Scheduler information:");

    vga_write("  Initialized: ");
    vga_write_line(stats->initialized ? "yes" : "no");

    vga_write("  Active tasks: ");
    vga_write_u32(stats->active_tasks);
    vga_write_line("");

    vga_write("  Running tasks: ");
    vga_write_u32(stats->running_tasks);
    vga_write_line("");

    vga_write("  Completed tasks: ");
    vga_write_u32(stats->completed_tasks);
    vga_write_line("");

    vga_write("  Disabled tasks: ");
    vga_write_u32(stats->disabled_tasks);
    vga_write_line("");

    vga_write("  Sleeping tasks: ");
    vga_write_u32(stats->sleeping_tasks);
    vga_write_line("");

    vga_write("  Total created tasks: ");
    vga_write_u32(stats->total_created_tasks);
    vga_write_line("");

    vga_write("  Scheduler runs: ");
    vga_write_u32(stats->scheduler_runs);
    vga_write_line("");

    vga_write("  Task dispatches: ");
    vga_write_u32(stats->task_dispatches);
    vga_write_line("");

    vga_write("  Manual dispatches: ");
    vga_write_u32(stats->manual_dispatches);
    vga_write_line("");

    vga_write("  Switched dispatches: ");
    vga_write_u32(stats->switched_dispatches);
    vga_write_line("");

    vga_write("  Switched returns: ");
    vga_write_u32(stats->switched_returns);
    vga_write_line("");

    vga_write("  Current task index: ");
    vga_write_u32(stats->current_task_index);
    vga_write_line("");

    vga_write("  Last selected task id: ");
    if (stats->last_selected_task_id == SCHEDULER_NO_TASK_SELECTED) {
        vga_write_line("none");
    } else {
        vga_write_u32(stats->last_selected_task_id);
        vga_write_line("");
    }

    vga_write("  Round-robin passes: ");
    vga_write_u32(stats->round_robin_passes);
    vga_write_line("");

    vga_write("  Tasks considered: ");
    vga_write_u32(stats->tasks_considered);
    vga_write_line("");

    vga_write("  Tasks selected: ");
    vga_write_u32(stats->tasks_selected);
    vga_write_line("");

    vga_write("  No task selected: ");
    vga_write_u32(stats->no_task_selected);
    vga_write_line("");

    const scheduler_ready_queue_info_t* queue = scheduler_get_ready_queue_info();

    vga_write("  Ready queue rebuilds: ");
    vga_write_u32(queue->rebuilds);
    vga_write_line("");

    vga_write("  Ready queue count: ");
    vga_write_u32(queue->count);
    vga_write_line("");

    vga_write("  Ready queue overflows: ");
    vga_write_u32(queue->overflows);
    vga_write_line("");

    vga_write("  Trampoline entries: ");
    vga_write_u32(stats->trampoline_entries);
    vga_write_line("");

    vga_write("  Task completions: ");
    vga_write_u32(stats->task_completions);
    vga_write_line("");

    vga_write("  Task sleeps: ");
    vga_write_u32(stats->task_sleeps);
    vga_write_line("");

    vga_write("  Task wakes: ");
    vga_write_u32(stats->task_wakes);
    vga_write_line("");

    vga_write("  Task yields: ");
    vga_write_u32(stats->task_yields);
    vga_write_line("");

    vga_write("  Failed task yields: ");
    vga_write_u32(stats->failed_task_yields);
    vga_write_line("");

    vga_write("  Last yielded task id: ");
    vga_write_u32(stats->last_yielded_task_id);
    vga_write_line("");

    vga_write("  Context self-tests: ");
    vga_write_u32(stats->context_self_tests);
    vga_write_line("");

    vga_write("  Switch tests: ");
    vga_write_u32(stats->switch_tests);
    vga_write_line("");

    vga_write("  Switch test failures: ");
    vga_write_u32(stats->switch_test_failures);
    vga_write_line("");

    const scheduler_preempt_test_info_t* preempt = scheduler_get_preempt_test_info();

    vga_write("  Preempt test enabled: ");
    vga_write_line(preempt->enabled ? "yes" : "no");

    vga_write("  Preempt timer dispatches: ");
    vga_write_u32(preempt->timer_dispatches);
    vga_write_line("");

    vga_write("  Allocated kernel stacks: ");
    vga_write_u32(stats->allocated_kernel_stacks);
    vga_write_line("");

    vga_write("  Freed kernel stacks: ");
    vga_write_u32(stats->freed_kernel_stacks);
    vga_write_line("");

    vga_write("  Failed stack allocations: ");
    vga_write_u32(stats->failed_stack_allocations);
    vga_write_line("");

    vga_write("  Test workload active: ");
    vga_write_line(scheduler_is_test_workload_active() ? "yes" : "no");

    vga_write("  Test workload counter: ");
    vga_write_u32(scheduler_get_test_workload_counter());
    vga_write_line("");

    vga_write("  Shell task counter: ");
    vga_write_u32(shell_created_task_counter);
    vga_write_line("");

    vga_write("  Shell yield workload counter: ");
    vga_write_u32(shell_yield_workload_counter);
    vga_write_line("");
}

static void shell_print_scheduler_policy(void) {
    const scheduler_stats_t* stats = scheduler_get_stats();

    vga_write_line("Scheduler policy:");

    vga_write_line("  Policy: round-robin");
    vga_write_line("  Dispatch model: one ready task per scheduler pass");
    vga_write_line("  Preemption: timer test mode only");

    vga_write("  Current cursor index: ");
    vga_write_u32(stats->current_task_index);
    vga_write_line("");

    vga_write("  Last selected task id: ");
    if (stats->last_selected_task_id == SCHEDULER_NO_TASK_SELECTED) {
        vga_write_line("none");
    } else {
        vga_write_u32(stats->last_selected_task_id);
        vga_write_line("");
    }

    vga_write("  Round-robin passes: ");
    vga_write_u32(stats->round_robin_passes);
    vga_write_line("");

    vga_write("  Tasks considered: ");
    vga_write_u32(stats->tasks_considered);
    vga_write_line("");

    vga_write("  Tasks selected: ");
    vga_write_u32(stats->tasks_selected);
    vga_write_line("");

    vga_write("  Empty selections: ");
    vga_write_u32(stats->no_task_selected);
    vga_write_line("");

    const scheduler_ready_queue_info_t* queue = scheduler_get_ready_queue_info();

    vga_write("  Ready queue rebuilds: ");
    vga_write_u32(queue->rebuilds);
    vga_write_line("");

    vga_write("  Ready queue count: ");
    vga_write_u32(queue->count);
    vga_write_line("");

    vga_write("  Task yields: ");
    vga_write_u32(stats->task_yields);
    vga_write_line("");

    vga_write("  Last yielded task id: ");
    vga_write_u32(stats->last_yielded_task_id);
    vga_write_line("");
}

static void shell_print_ready_queue(void) {
    const scheduler_ready_queue_info_t* queue = scheduler_get_ready_queue_info();

    vga_write_line("Ready queue snapshot:");

    vga_write("  Rebuilds: ");
    vga_write_u32(queue->rebuilds);
    vga_write_line("");

    vga_write("  Count: ");
    vga_write_u32(queue->count);
    vga_write_line("");

    vga_write("  Head: ");
    vga_write_u32(queue->head);
    vga_write_line("");

    vga_write("  Tail: ");
    vga_write_u32(queue->tail);
    vga_write_line("");

    vga_write("  Overflows: ");
    vga_write_u32(queue->overflows);
    vga_write_line("");

    vga_write("  Selected slot: ");
    if (queue->selected_slot == SCHEDULER_NO_TASK_SELECTED) {
        vga_write_line("none");
    } else {
        vga_write_u32(queue->selected_slot);
        vga_write_line("");
    }

    vga_write_line("  Task IDs:");

    if (queue->count == 0) {
        vga_write_line("    none");
        return;
    }

    for (uint32_t i = 0; i < queue->count; i++) {
        vga_write("    ");
        vga_write_u32(i);
        vga_write(": task ");
        vga_write_u32(queue->task_ids[i]);
        vga_write_line("");
    }
}

static void shell_print_scheduler_audit(void) {
    const scheduler_audit_info_t* audit = scheduler_get_audit_info();

    vga_write_line("Scheduler audit:");

    vga_write("  Audits run: ");
    vga_write_u32(audit->audits_run);
    vga_write_line("");

    vga_write("  Repairs run: ");
    vga_write_u32(audit->repairs_run);
    vga_write_line("");

    vga_write("  Issues found: ");
    vga_write_u32(audit->issues_found);
    vga_write_line("");

    vga_write("  Issues repaired: ");
    vga_write_u32(audit->issues_repaired);
    vga_write_line("");

    vga_write("  Invalid used state: ");
    vga_write_u32(audit->invalid_used_state);
    vga_write_line("");

    vga_write("  Unused task with stack: ");
    vga_write_u32(audit->unused_task_with_stack);
    vga_write_line("");

    vga_write("  Active task without stack: ");
    vga_write_u32(audit->active_task_without_stack);
    vga_write_line("");

    vga_write("  Active task without entry: ");
    vga_write_u32(audit->active_task_without_entry);
    vga_write_line("");

    vga_write("  Sleeping task without wake tick: ");
    vga_write_u32(audit->sleeping_task_without_wake_tick);
    vga_write_line("");

    vga_write("  Inactive task with stack: ");
    vga_write_u32(audit->inactive_task_with_stack);
    vga_write_line("");

    vga_write("  Ready queue invalid slot: ");
    vga_write_u32(audit->ready_queue_invalid_slot);
    vga_write_line("");

    vga_write("  Ready queue task id mismatch: ");
    vga_write_u32(audit->ready_queue_task_id_mismatch);
    vga_write_line("");

    vga_write("  Ready queue over capacity: ");
    vga_write_u32(audit->ready_queue_over_capacity);
    vga_write_line("");

    vga_write("  Current task invalid: ");
    vga_write_u32(audit->current_task_invalid);
    vga_write_line("");

    vga_write("  Switched task invalid: ");
    vga_write_u32(audit->switched_task_invalid);
    vga_write_line("");
}

static void shell_print_scheduler_lock_status(void) {
    const scheduler_lock_info_t* lock = scheduler_get_lock_info();

    vga_write_line("Scheduler lock:");

    vga_write("  Locked: ");
    vga_write_line(lock->locked ? "yes" : "no");

    vga_write("  Interrupts enabled now: ");
    vga_write_line(lock->interrupts_enabled_now ? "yes" : "no");

    vga_write("  Depth: ");
    vga_write_u32(lock->depth);
    vga_write_line("");

    vga_write("  Saved flags: ");
    vga_write_hex_u32(lock->saved_flags);
    vga_write_line("");

    vga_write("  Lock count: ");
    vga_write_u32(lock->lock_count);
    vga_write_line("");

    vga_write("  Unlock count: ");
    vga_write_u32(lock->unlock_count);
    vga_write_line("");

    vga_write("  Max depth: ");
    vga_write_u32(lock->max_depth);
    vga_write_line("");

    vga_write("  Failed unlocks: ");
    vga_write_u32(lock->failed_unlocks);
    vga_write_line("");

    vga_write("  Test runs: ");
    vga_write_u32(lock->test_runs);
    vga_write_line("");
}

static void shell_print_status_code(kernel_status_t status) {
    vga_write("  ");

    if (status < 0) {
        vga_write("-");
        vga_write_u32((uint32_t)(0 - status));
    } else {
        vga_write_u32((uint32_t)status);
    }

    vga_write(": ");
    vga_write_line(kernel_status_to_string(status));
}

static void shell_print_status_table(void) {
    vga_write_line("Kernel status codes:");

    shell_print_status_code(KERNEL_OK);
    shell_print_status_code(KERNEL_ERROR_UNKNOWN);
    shell_print_status_code(KERNEL_ERROR_INVALID_ARGUMENT);
    shell_print_status_code(KERNEL_ERROR_NOT_FOUND);
    shell_print_status_code(KERNEL_ERROR_OUT_OF_MEMORY);
    shell_print_status_code(KERNEL_ERROR_ALREADY_EXISTS);
    shell_print_status_code(KERNEL_ERROR_NOT_INITIALIZED);
    shell_print_status_code(KERNEL_ERROR_BUSY);
    shell_print_status_code(KERNEL_ERROR_PERMISSION_DENIED);
    shell_print_status_code(KERNEL_ERROR_UNSUPPORTED);
    shell_print_status_code(KERNEL_ERROR_INVALID_STATE);
    shell_print_status_code(KERNEL_ERROR_CORRUPTION_DETECTED);
    shell_print_status_code(KERNEL_ERROR_TIMEOUT);
}

static void shell_handle_status_is(const char* args) {
    uint32_t raw_value = 0;
    kernel_status_t status = 0;

    if (args != 0 && args[0] == '-') {
        if (!string_to_uint32_auto(args + 1, &raw_value)) {
            vga_write_line("Usage: status-is <code>");
            vga_write_line("Example: status-is -4");
            return;
        }

        status = (kernel_status_t)(0 - raw_value);
    } else {
        if (!string_to_uint32_auto(args, &raw_value)) {
            vga_write_line("Usage: status-is <code>");
            vga_write_line("Example: status-is -4");
            return;
        }

        status = (kernel_status_t)raw_value;
    }

    vga_write_line("Kernel status:");
    shell_print_status_code(status);

    vga_write("  Success: ");
    vga_write_line(kernel_status_is_success(status) ? "yes" : "no");

    vga_write("  Error: ");
    vga_write_line(kernel_status_is_error(status) ? "yes" : "no");
}

static void shell_print_tasks(void) {
    const scheduler_task_t* tasks = scheduler_get_tasks();
    uint32_t count = 0;

    vga_write_line("Scheduler tasks:");

    for (uint32_t i = 0; i < SCHEDULER_MAX_TASKS; i++) {
        if (!tasks[i].used) {
            continue;
        }

        vga_write("  id=");
        vga_write_u32(tasks[i].id);

        vga_write(" name=");
        vga_write(tasks[i].name);

        vga_write(" state=");
        vga_write(shell_scheduler_state_name(tasks[i].state));

        vga_write(" mode=");
        vga_write(shell_scheduler_mode_name(tasks[i].mode));

        vga_write(" interval=");
        vga_write_u32(tasks[i].interval_ticks);

        vga_write(" runs=");
        vga_write_u32(tasks[i].run_count);

        vga_write(" wake=");
        vga_write_u32(tasks[i].wake_tick);

        vga_write_line("");

        vga_write("     stack=");
        vga_write_hex_u32((uint32_t)tasks[i].kernel_stack_base);

        vga_write(" top=");
        vga_write_hex_u32(tasks[i].kernel_stack_top);

        vga_write(" eip=");
        vga_write_hex_u32(tasks[i].context.eip);

        vga_write_line("");

        count++;
    }

    if (count == 0) {
        vga_write_line("  none");
    }
}

static void shell_print_context(void) {
    uint32_t eip = context_read_eip();
    uint32_t esp = context_read_esp();
    uint32_t ebp = context_read_ebp();
    uint32_t eflags = context_read_eflags();

    vga_write_line("Current CPU context:");

    vga_write("  EIP: ");
    vga_write_hex_u32(eip);
    vga_write_line("");

    vga_write("  ESP: ");
    vga_write_hex_u32(esp);
    vga_write_line("");

    vga_write("  EBP: ");
    vga_write_hex_u32(ebp);
    vga_write_line("");

    vga_write("  EFLAGS: ");
    vga_write_hex_u32(eflags);
    vga_write_line("");

    vga_write("  Context switch routine: ");
    vga_write_hex_u32((uint32_t)scheduler_context_switch);
    vga_write_line("");

    vga_write_line("  Real task switching: shell-controlled only");
    vga_write_line("  Full task context includes EAX/EBX/ECX/EDX/ESI/EDI");
}

static void shell_print_switch_status(void) {
    const scheduler_switch_test_info_t* info = scheduler_get_switch_test_info();

    vga_write_line("Context switch test status:");

    vga_write("  Stack allocated: ");
    vga_write_line(info->stack_allocated ? "yes" : "no");

    vga_write("  Started: ");
    vga_write_line(info->started ? "yes" : "no");

    vga_write("  Entered test context: ");
    vga_write_line(info->entered ? "yes" : "no");

    vga_write("  Completed round trip: ");
    vga_write_line(info->completed ? "yes" : "no");

    vga_write("  Returned unexpectedly: ");
    vga_write_line(info->returned_unexpectedly ? "yes" : "no");

    vga_write("  Run count: ");
    vga_write_u32(info->run_count);
    vga_write_line("");

    vga_write("  Test stack base: ");
    vga_write_hex_u32(info->test_stack_base);
    vga_write_line("");

    vga_write("  Test stack top: ");
    vga_write_hex_u32(info->test_stack_top);
    vga_write_line("");

    vga_write("  Test entry: ");
    vga_write_hex_u32(info->test_entry_address);
    vga_write_line("");

    vga_write("  Return EIP: ");
    vga_write_hex_u32(info->return_eip);
    vga_write_line("");

    vga_write("  Test saved EIP: ");
    vga_write_hex_u32(info->test_saved_eip);
    vga_write_line("");
    vga_write_line("  Full register save/restore: enabled");
}

static void shell_print_preempt_status(void) {
    const scheduler_preempt_test_info_t* info = scheduler_get_preempt_test_info();

    vga_write_line("Timer scheduler test status:");

    vga_write("  Enabled: ");
    vga_write_line(info->enabled ? "yes" : "no");

    vga_write("  In timer dispatch: ");
    vga_write_line(info->in_timer_dispatch ? "yes" : "no");

    vga_write("  Tick interval: ");
    vga_write_u32(info->tick_interval);
    vga_write_line("");

    vga_write("  Last dispatch tick: ");
    vga_write_u32(info->last_dispatch_tick);
    vga_write_line("");

    vga_write("  Timer ticks seen: ");
    vga_write_u32(info->timer_ticks_seen);
    vga_write_line("");

    vga_write("  Timer dispatches: ");
    vga_write_u32(info->timer_dispatches);
    vga_write_line("");

    vga_write("  Skipped reentrant dispatches: ");
    vga_write_u32(info->skipped_reentrant_dispatches);
    vga_write_line("");
}

static uint32_t shell_count_tracked_pmm_allocations(void) {
    uint32_t count = 0;

    for (uint32_t i = 0; i < SHELL_MAX_TRACKED_PMM_ALLOCATIONS; i++) {
        if (pmm_allocations[i].active) {
            count++;
        }
    }

    return count;
}

static uint32_t shell_count_tracked_heap_allocations(void) {
    uint32_t count = 0;

    for (uint32_t i = 0; i < SHELL_MAX_TRACKED_HEAP_ALLOCATIONS; i++) {
        if (heap_allocations[i].active) {
            count++;
        }
    }

    return count;
}

static void shell_print_memory(void) {
    const pmm_stats_t* stats = pmm_get_stats();

    vga_write("Usable memory KB: ");
    vga_write_u32((unsigned int)(stats->usable_memory_bytes / 1024));
    vga_write_line("");

    vga_write("Allocated pages: ");
    vga_write_u32(stats->allocated_pages);
    vga_write_line("");

    vga_write("Free stack pages: ");
    vga_write_u32(stats->free_stack_count);
    vga_write_line("");

    vga_write("Tracked PMM allocations: ");
    vga_write_u32(shell_count_tracked_pmm_allocations());
    vga_write_line("");

    vga_write("Next free page: ");
    vga_write_hex_u32(stats->next_free_page);
    vga_write_line("");

    vga_write("Allocation range: ");
    vga_write_hex_u32(stats->allocation_start);
    vga_write(" - ");
    vga_write_hex_u32(stats->allocation_end);
    vga_write_line("");
}

static void shell_print_heap(void) {
    const heap_stats_t* stats = heap_get_stats();

    vga_write_line("Kernel heap information:");

    vga_write("  Initialized: ");
    vga_write_line(stats->initialized ? "yes" : "no");

    vga_write("  Virtual start: ");
    vga_write_hex_u32(stats->virtual_start);
    vga_write_line("");

    vga_write("  Virtual end: ");
    vga_write_hex_u32(stats->virtual_end);
    vga_write_line("");

    vga_write("  Next virtual page: ");
    vga_write_hex_u32(stats->next_virtual_page);
    vga_write_line("");

    vga_write("  Total pages: ");
    vga_write_u32(stats->total_pages);
    vga_write_line("");

    vga_write("  Total bytes: ");
    vga_write_u32(stats->total_bytes);
    vga_write_line("");

    vga_write("  Used bytes: ");
    vga_write_u32(stats->used_bytes);
    vga_write_line("");

    vga_write("  Free bytes: ");
    vga_write_u32(stats->free_bytes);
    vga_write_line("");

    vga_write("  Allocations: ");
    vga_write_u32(stats->allocation_count);
    vga_write_line("");

    vga_write("  Free blocks: ");
    vga_write_u32(stats->free_block_count);
    vga_write_line("");

    vga_write("  Grow count: ");
    vga_write_u32(stats->grow_count);
    vga_write_line("");

    vga_write("  Failed grows: ");
    vga_write_u32(stats->failed_grow_count);
    vga_write_line("");

    vga_write("  Failed allocations: ");
    vga_write_u32(stats->failed_allocation_count);
    vga_write_line("");

    vga_write("  Successful frees: ");
    vga_write_u32(stats->successful_free_count);
    vga_write_line("");

    vga_write("  Invalid frees: ");
    vga_write_u32(stats->invalid_free_count);
    vga_write_line("");

    vga_write("  Double frees: ");
    vga_write_u32(stats->double_free_count);
    vga_write_line("");

    vga_write("  Tracked allocations: ");
    vga_write_u32(shell_count_tracked_heap_allocations());
    vga_write_line("");
}

static void shell_print_heap_audit(void) {
    uint32_t issues = heap_audit();
    const heap_audit_info_t* audit = heap_get_audit_info();

    if (issues == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_write_line("Heap audit passed");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write("Heap audit found issues: ");
        vga_write_u32(issues);
        vga_write_line("");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }

    vga_write_line("Heap audit:");

    vga_write("  Audits run: ");
    vga_write_u32(audit->audits_run);
    vga_write_line("");

    vga_write("  Issues found: ");
    vga_write_u32(audit->issues_found);
    vga_write_line("");

    vga_write("  Blocks seen: ");
    vga_write_u32(audit->blocks_seen);
    vga_write_line("");

    vga_write("  Used blocks: ");
    vga_write_u32(audit->used_blocks);
    vga_write_line("");

    vga_write("  Free blocks: ");
    vga_write_u32(audit->free_blocks);
    vga_write_line("");

    vga_write("  Invalid magic blocks: ");
    vga_write_u32(audit->invalid_magic_blocks);
    vga_write_line("");

    vga_write("  Out-of-range blocks: ");
    vga_write_u32(audit->out_of_range_blocks);
    vga_write_line("");

    vga_write("  Misaligned blocks: ");
    vga_write_u32(audit->misaligned_blocks);
    vga_write_line("");

    vga_write("  Invalid size blocks: ");
    vga_write_u32(audit->invalid_size_blocks);
    vga_write_line("");

    vga_write("  Invalid next links: ");
    vga_write_u32(audit->invalid_next_links);
    vga_write_line("");

    vga_write("  Unmapped heap pages: ");
    vga_write_u32(audit->unmapped_heap_pages);
    vga_write_line("");

    vga_write("  Calculated used bytes: ");
    vga_write_u32(audit->calculated_used_bytes);
    vga_write_line("");

    vga_write("  Calculated free bytes: ");
    vga_write_u32(audit->calculated_free_bytes);
    vga_write_line("");
}

static void shell_handle_heap_validate(const char* args) {
    uint32_t address = 0;

    if (!string_to_uint32_auto(args, &address) || address == 0) {
        vga_write_line("Usage: heap-validate <address>");
        return;
    }

    vga_write("Heap pointer: ");
    vga_write_hex_u32(address);
    vga_write_line("");

    vga_write("  Valid allocated pointer: ");
    vga_write_line(heap_validate_pointer((void*)address) ? "yes" : "no");
}

static void shell_print_vmm(void) {
    const vmm_stats_t* stats = vmm_get_stats();

    vga_write_line("Virtual memory manager information:");

    vga_write("  Initialized: ");
    vga_write_line(stats->initialized ? "yes" : "no");

    vga_write("  Runtime mapped pages: ");
    vga_write_u32(stats->mapped_pages);
    vga_write_line("");

    vga_write("  Created page tables: ");
    vga_write_u32(stats->created_page_tables);
    vga_write_line("");

    vga_write("  Last mapped virtual: ");
    vga_write_hex_u32(stats->last_mapped_virtual_address);
    vga_write_line("");

    vga_write("  Last mapped physical: ");
    vga_write_hex_u32(stats->last_mapped_physical_address);
    vga_write_line("");

    vga_write("  Failed maps: ");
    vga_write_u32(stats->failed_map_count);
    vga_write_line("");

    vga_write("  Failed unmaps: ");
    vga_write_u32(stats->failed_unmap_count);
    vga_write_line("");

    vga_write("  Remaps: ");
    vga_write_u32(stats->remap_count);
    vga_write_line("");

    vga_write("  Invalid maps: ");
    vga_write_u32(stats->invalid_map_count);
    vga_write_line("");

    vga_write("  Missing unmaps: ");
    vga_write_u32(stats->missing_unmap_count);
    vga_write_line("");

    vga_write("  Test mapping active: ");
    vga_write_line(vmm_test_mapping_active ? "yes" : "no");

    if (vmm_test_physical_page != 0) {
        vga_write("  Test physical page: ");
        vga_write_hex_u32((uint32_t)vmm_test_physical_page);
        vga_write_line("");
    }
}

static void shell_print_vmm_audit(void) {
    uint32_t issues = vmm_audit();
    const vmm_audit_info_t* audit = vmm_get_audit_info();

    if (issues == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_write_line("VMM audit passed");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write("VMM audit found issues: ");
        vga_write_u32(issues);
        vga_write_line("");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }

    vga_write_line("VMM audit:");

    vga_write("  Audits run: ");
    vga_write_u32(audit->audits_run);
    vga_write_line("");

    vga_write("  Issues found: ");
    vga_write_u32(audit->issues_found);
    vga_write_line("");

    vga_write("  Present directory entries: ");
    vga_write_u32(audit->present_directory_entries);
    vga_write_line("");

    vga_write("  Present page entries: ");
    vga_write_u32(audit->present_page_entries);
    vga_write_line("");

    vga_write("  Writable page entries: ");
    vga_write_u32(audit->writable_page_entries);
    vga_write_line("");

    vga_write("  User page entries: ");
    vga_write_u32(audit->user_page_entries);
    vga_write_line("");

    vga_write("  Invalid directory entries: ");
    vga_write_u32(audit->invalid_directory_entries);
    vga_write_line("");

    vga_write("  Invalid page table addresses: ");
    vga_write_u32(audit->invalid_page_table_addresses);
    vga_write_line("");

    vga_write("  Last mapping missing: ");
    vga_write_u32(audit->last_mapping_missing);
    vga_write_line("");
}

static void shell_print_paging(void) {
    const paging_info_t* info = paging_get_info();

    vga_write_line("Paging information:");

    vga_write("  Enabled: ");
    vga_write_line(info->enabled ? "yes" : "no");

    vga_write("  Page tables: ");
    vga_write_u32(info->page_table_count);
    vga_write_line("");

    vga_write("  Mapped start: ");
    vga_write_hex_u32(info->mapped_start);
    vga_write_line("");

    vga_write("  Mapped end: ");
    vga_write_hex_u32(info->mapped_end);
    vga_write_line("");

    vga_write("  Mapped KB: ");
    vga_write_u32(info->mapped_bytes / 1024);
    vga_write_line("");

    vga_write("  Page directory: ");
    vga_write_hex_u32(info->page_directory_address);
    vga_write_line("");
}

static void shell_print_layout(void) {
    vga_write_line("Kernel virtual memory layout:");

    vga_write("  Identity map: ");
    vga_write_hex_u32(MEMORY_LAYOUT_IDENTITY_START);
    vga_write(" - ");
    vga_write_hex_u32(MEMORY_LAYOUT_IDENTITY_END);
    vga_write_line("");

    vga_write("  Kernel base: ");
    vga_write_hex_u32(MEMORY_LAYOUT_KERNEL_BASE);
    vga_write_line("");

    vga_write("  VMM test page: ");
    vga_write_hex_u32(MEMORY_LAYOUT_VMM_TEST_PAGE);
    vga_write_line("");

    vga_write("  Higher-half start: ");
    vga_write_hex_u32(MEMORY_LAYOUT_HIGHER_HALF_START);
    vga_write_line("");

    vga_write("  Kernel heap: ");
    vga_write_hex_u32(MEMORY_LAYOUT_KERNEL_HEAP_START);
    vga_write(" - ");
    vga_write_hex_u32(MEMORY_LAYOUT_KERNEL_HEAP_END);
    vga_write_line("");
}

static void shell_handle_virt(const char* args) {
    uint32_t virtual_address = 0;

    if (!string_to_uint32_auto(args, &virtual_address)) {
        vga_write_line("Usage: virt <address>");
        vga_write_line("Example: virt 0xC1000000");
        return;
    }

    uint32_t aligned_virtual = virtual_address & 0xFFFFF000;
    uint32_t offset = virtual_address & 0x00000FFF;
    int mapped = vmm_is_mapped(virtual_address);

    vga_write_line("Virtual address information:");

    vga_write("  Virtual address: ");
    vga_write_hex_u32(virtual_address);
    vga_write_line("");

    vga_write("  Page base: ");
    vga_write_hex_u32(aligned_virtual);
    vga_write_line("");

    vga_write("  Page offset: ");
    vga_write_hex_u32(offset);
    vga_write_line("");

    vga_write("  Mapped: ");
    vga_write_line(mapped ? "yes" : "no");

    if (mapped) {
        uint32_t physical_address = vmm_get_physical_address(virtual_address);

        vga_write("  Physical address: ");
        vga_write_hex_u32(physical_address);
        vga_write_line("");
    }
}

static void shell_print_ticks(void) {
    vga_write("Timer ticks: ");
    vga_write_u32(timer_get_ticks());
    vga_write_line("");
}

static void shell_handle_vmap_test(void) {
    uint32_t virtual_address = MEMORY_LAYOUT_VMM_TEST_PAGE;

    if (vmm_test_mapping_active) {
        vga_write_line("VMM test mapping already active");
        return;
    }

    void* physical_page = pmm_alloc_page();

    if (physical_page == 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to allocate physical page for VMM test");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    if (!vmm_map_page(
        virtual_address,
        (uint32_t)physical_page,
        VMM_PAGE_WRITABLE
    )) {
        pmm_free_page(physical_page);
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to map VMM test page");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    volatile uint32_t* mapped_value = (volatile uint32_t*)virtual_address;
    *mapped_value = 0x4C49414D;

    vmm_test_physical_page = physical_page;
    vmm_test_mapping_active = 1;

    vga_write_line("Mapped virtual page successfully");

    vga_write("  Virtual address: ");
    vga_write_hex_u32(virtual_address);
    vga_write_line("");

    vga_write("  Physical page: ");
    vga_write_hex_u32((uint32_t)physical_page);
    vga_write_line("");

    vga_write("  Written value: ");
    vga_write_hex_u32(*mapped_value);
    vga_write_line("");
}

static void shell_handle_vunmap_test(void) {
    uint32_t virtual_address = MEMORY_LAYOUT_VMM_TEST_PAGE;

    if (!vmm_test_mapping_active) {
        vga_write_line("No active VMM test mapping");
        return;
    }

    if (!vmm_unmap_page(virtual_address)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to unmap VMM test page");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    pmm_free_page(vmm_test_physical_page);

    vga_write_line("Unmapped VMM test page");

    vga_write("  Virtual address: ");
    vga_write_hex_u32(virtual_address);
    vga_write_line("");

    vga_write("  Freed physical page: ");
    vga_write_hex_u32((uint32_t)vmm_test_physical_page);
    vga_write_line("");

    vmm_test_physical_page = 0;
    vmm_test_mapping_active = 0;
}

static void shell_print_uptime(void) {
    uint32_t ticks = timer_get_ticks();
    uint32_t frequency = timer_get_frequency_hz();
    uint32_t seconds = 0;

    if (frequency != 0) {
        seconds = ticks / frequency;
    }

    vga_write("Uptime seconds: ");
    vga_write_u32(seconds);
    vga_write_line("");

    vga_write("Raw ticks: ");
    vga_write_u32(ticks);
    vga_write_line("");
}

static int shell_set_color(const char* color_name) {
    if (string_equals(color_name, "white")) {
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return 1;
    }

    if (string_equals(color_name, "green")) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        return 1;
    }

    if (string_equals(color_name, "cyan")) {
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        return 1;
    }

    if (string_equals(color_name, "red")) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        return 1;
    }

    if (string_equals(color_name, "yellow")) {
        vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
        return 1;
    }

    if (string_equals(color_name, "gray")) {
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return 1;
    }

    return 0;
}

static void shell_handle_color(const char* args) {
    if (string_equals(args, "")) {
        vga_write_line("Usage: color <white|green|cyan|red|yellow|gray>");
        return;
    }

    if (!shell_set_color(args)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write("Unknown color: ");
        vga_write_line(args);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    vga_write("Shell color set to ");
    vga_write_line(args);
}

static int shell_find_free_pmm_tracking_slot(void) {
    for (uint32_t i = 0; i < SHELL_MAX_TRACKED_PMM_ALLOCATIONS; i++) {
        if (!pmm_allocations[i].active) {
            return (int)i;
        }
    }

    return -1;
}

static int shell_find_latest_pmm_tracking_slot(void) {
    for (int i = SHELL_MAX_TRACKED_PMM_ALLOCATIONS - 1; i >= 0; i--) {
        if (pmm_allocations[i].active) {
            return i;
        }
    }

    return -1;
}

static int shell_find_free_heap_tracking_slot(void) {
    for (uint32_t i = 0; i < SHELL_MAX_TRACKED_HEAP_ALLOCATIONS; i++) {
        if (!heap_allocations[i].active) {
            return (int)i;
        }
    }

    return -1;
}

static int shell_find_latest_heap_tracking_slot(void) {
    for (int i = SHELL_MAX_TRACKED_HEAP_ALLOCATIONS - 1; i >= 0; i--) {
        if (heap_allocations[i].active) {
            return i;
        }
    }

    return -1;
}

static void shell_handle_alloc(const char* args) {
    uint32_t pages = 0;

    if (!string_to_uint32(args, &pages) || pages == 0) {
        vga_write_line("Usage: alloc <positive page count>");
        return;
    }

    int slot = shell_find_free_pmm_tracking_slot();

    if (slot < 0) {
        vga_write_line("PMM tracking table is full. Run pmm-free-all first.");
        return;
    }

    void* first_page = 0;
    void* current_page = 0;

    for (uint32_t i = 0; i < pages; i++) {
        current_page = pmm_alloc_page();

        if (current_page == 0) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_write_line("Allocation failed");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            return;
        }

        if (first_page == 0) {
            first_page = current_page;
        }
    }

    pmm_allocations[slot].address = first_page;
    pmm_allocations[slot].pages = pages;
    pmm_allocations[slot].active = 1;

    vga_write("Allocated PMM pages: ");
    vga_write_u32(pages);
    vga_write_line("");

    vga_write("First page: ");
    vga_write_hex_u32((uint32_t)first_page);
    vga_write_line("");

    vga_write("Last page: ");
    vga_write_hex_u32((uint32_t)current_page);
    vga_write_line("");

    vga_write("Tracking slot: ");
    vga_write_u32((uint32_t)slot);
    vga_write_line("");
}

static void shell_print_pmm_list(void) {
    uint32_t count = 0;

    vga_write_line("Tracked PMM allocations:");

    for (uint32_t i = 0; i < SHELL_MAX_TRACKED_PMM_ALLOCATIONS; i++) {
        if (!pmm_allocations[i].active) {
            continue;
        }

        vga_write("  [");
        vga_write_u32(i);
        vga_write("] ");
        vga_write_hex_u32((uint32_t)pmm_allocations[i].address);
        vga_write(" pages=");
        vga_write_u32(pmm_allocations[i].pages);
        vga_write_line("");

        count++;
    }

    if (count == 0) {
        vga_write_line("  none");
    }
}

static void shell_free_pmm_slot(uint32_t slot) {
    if (slot >= SHELL_MAX_TRACKED_PMM_ALLOCATIONS || !pmm_allocations[slot].active) {
        return;
    }

    uint32_t start = (uint32_t)pmm_allocations[slot].address;

    for (uint32_t i = 0; i < pmm_allocations[slot].pages; i++) {
        pmm_free_page((void*)(start + (i * PMM_PAGE_SIZE)));
    }

    pmm_allocations[slot].address = 0;
    pmm_allocations[slot].pages = 0;
    pmm_allocations[slot].active = 0;
}

static void shell_handle_free_last(void) {
    int slot = shell_find_latest_pmm_tracking_slot();

    if (slot < 0) {
        vga_write_line("No tracked PMM allocation to free");
        return;
    }

    shell_free_pmm_slot((uint32_t)slot);

    vga_write("Freed PMM tracking slot: ");
    vga_write_u32((uint32_t)slot);
    vga_write_line("");
}

static void shell_handle_pmm_free_all(void) {
    uint32_t freed = 0;

    for (uint32_t i = 0; i < SHELL_MAX_TRACKED_PMM_ALLOCATIONS; i++) {
        if (pmm_allocations[i].active) {
            shell_free_pmm_slot(i);
            freed++;
        }
    }

    vga_write("Freed PMM allocations: ");
    vga_write_u32(freed);
    vga_write_line("");
}

static void shell_handle_kmalloc(const char* args) {
    uint32_t bytes = 0;

    if (!string_to_uint32(args, &bytes) || bytes == 0) {
        vga_write_line("Usage: kmalloc <positive byte count>");
        return;
    }

    int slot = shell_find_free_heap_tracking_slot();

    if (slot < 0) {
        vga_write_line("Heap tracking table is full. Run heap-free-all first.");
        return;
    }

    void* allocation = kmalloc(bytes);

    if (allocation == 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("kmalloc failed");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    heap_allocations[slot].address = allocation;
    heap_allocations[slot].bytes = bytes;
    heap_allocations[slot].active = 1;

    vga_write("Allocated heap bytes: ");
    vga_write_u32(bytes);
    vga_write_line("");

    vga_write("Heap allocation address: ");
    vga_write_hex_u32((uint32_t)allocation);
    vga_write_line("");

    vga_write("Tracking slot: ");
    vga_write_u32((uint32_t)slot);
    vga_write_line("");
}

static void shell_print_heap_list(void) {
    uint32_t count = 0;

    vga_write_line("Tracked heap allocations:");

    for (uint32_t i = 0; i < SHELL_MAX_TRACKED_HEAP_ALLOCATIONS; i++) {
        if (!heap_allocations[i].active) {
            continue;
        }

        vga_write("  [");
        vga_write_u32(i);
        vga_write("] ");
        vga_write_hex_u32((uint32_t)heap_allocations[i].address);
        vga_write(" bytes=");
        vga_write_u32(heap_allocations[i].bytes);
        vga_write_line("");

        count++;
    }

    if (count == 0) {
        vga_write_line("  none");
    }
}

static void shell_free_heap_slot(uint32_t slot) {
    if (slot >= SHELL_MAX_TRACKED_HEAP_ALLOCATIONS || !heap_allocations[slot].active) {
        return;
    }

    kfree(heap_allocations[slot].address);

    heap_allocations[slot].address = 0;
    heap_allocations[slot].bytes = 0;
    heap_allocations[slot].active = 0;
}

static void shell_handle_kfree_last(void) {
    int slot = shell_find_latest_heap_tracking_slot();

    if (slot < 0) {
        vga_write_line("No tracked heap allocation to free");
        return;
    }

    shell_free_heap_slot((uint32_t)slot);

    vga_write("Freed heap tracking slot: ");
    vga_write_u32((uint32_t)slot);
    vga_write_line("");
}

static void shell_handle_heap_free_all(void) {
    uint32_t freed = 0;

    for (uint32_t i = 0; i < SHELL_MAX_TRACKED_HEAP_ALLOCATIONS; i++) {
        if (heap_allocations[i].active) {
            shell_free_heap_slot(i);
            freed++;
        }
    }

    vga_write("Freed heap allocations: ");
    vga_write_u32(freed);
    vga_write_line("");
}

static void shell_created_noop_task(void) {
    shell_created_task_counter++;
}

static void shell_yield_workload_task(void) {
    shell_yield_workload_counter++;
    scheduler_yield_current_task();
}

static void shell_print_task_detail(const scheduler_task_t* task) {
    if (task == 0 || !task->used) {
        vga_write_line("Task not found");
        return;
    }

    vga_write_line("Task detail:");

    vga_write("  ID: ");
    vga_write_u32(task->id);
    vga_write_line("");

    vga_write("  Name: ");
    vga_write_line(task->name);

    vga_write("  State: ");
    vga_write_line(shell_scheduler_state_name(task->state));

    vga_write("  Mode: ");
    vga_write_line(shell_scheduler_mode_name(task->mode));

    vga_write("  Interval ticks: ");
    vga_write_u32(task->interval_ticks);
    vga_write_line("");

    vga_write("  Last run tick: ");
    vga_write_u32(task->last_run_tick);
    vga_write_line("");

    vga_write("  Run count: ");
    vga_write_u32(task->run_count);
    vga_write_line("");

    vga_write("  Wake tick: ");
    vga_write_u32(task->wake_tick);
    vga_write_line("");

    vga_write("  Sleep count: ");
    vga_write_u32(task->sleep_count);
    vga_write_line("");

    vga_write("  Stack base: ");
    vga_write_hex_u32((uint32_t)task->kernel_stack_base);
    vga_write_line("");

    vga_write("  Stack top: ");
    vga_write_hex_u32(task->kernel_stack_top);
    vga_write_line("");

    vga_write("  Context EIP: ");
    vga_write_hex_u32(task->context.eip);
    vga_write_line("");

    vga_write("  Context ESP: ");
    vga_write_hex_u32(task->context.esp);
    vga_write_line("");

    vga_write("  Context EBP: ");
    vga_write_hex_u32(task->context.ebp);
    vga_write_line("");

    vga_write("  Context EFLAGS: ");
    vga_write_hex_u32(task->context.eflags);
    vga_write_line("");

    vga_write("  Context EAX: ");
    vga_write_hex_u32(task->context.eax);
    vga_write_line("");

    vga_write("  Context EBX: ");
    vga_write_hex_u32(task->context.ebx);
    vga_write_line("");

    vga_write("  Context ECX: ");
    vga_write_hex_u32(task->context.ecx);
    vga_write_line("");

    vga_write("  Context EDX: ");
    vga_write_hex_u32(task->context.edx);
    vga_write_line("");

    vga_write("  Context ESI: ");
    vga_write_hex_u32(task->context.esi);
    vga_write_line("");

    vga_write("  Context EDI: ");
    vga_write_hex_u32(task->context.edi);
    vga_write_line("");
}

static void shell_print_current_task(void) {
    const scheduler_task_t* task = scheduler_get_current_task();

    if (task == 0) {
        vga_write_line("No task is currently running");
        return;
    }

    shell_print_task_detail(task);
}

static void shell_handle_task_info(const char* args) {
    uint32_t task_id = 0;

    if (!string_to_uint32_auto(args, &task_id) || task_id == 0) {
        vga_write_line("Usage: task-info <task id>");
        return;
    }

    shell_print_task_detail(scheduler_get_task(task_id));
}

static void shell_handle_task_run_now(const char* args) {
    uint32_t task_id = 0;

    if (!string_to_uint32_auto(args, &task_id) || task_id == 0) {
        vga_write_line("Usage: task-run-now <task id>");
        return;
    }

    if (!scheduler_run_task_now(task_id)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to run task");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    vga_write("Ran task id: ");
    vga_write_u32(task_id);
    vga_write_line("");
}

static void shell_handle_task_switch_run(const char* args) {
    uint32_t task_id = 0;

    if (!string_to_uint32_auto(args, &task_id) || task_id == 0) {
        vga_write_line("Usage: task-switch-run <task id>");
        return;
    }

    if (!scheduler_run_task_with_context_switch(task_id)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to switch-run task");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    vga_write("Switch-ran task id: ");
    vga_write_u32(task_id);
    vga_write_line("");
}

static void shell_handle_preempt_enable_test(const char* args) {
    uint32_t interval = 0;

    if (!string_to_uint32_auto(args, &interval)) {
        interval = 10;
    }

    if (interval == 0) {
        interval = 10;
    }

    scheduler_enable_preempt_test(interval);

    vga_write("Timer scheduler test enabled, interval ticks: ");
    vga_write_u32(interval);
    vga_write_line("");
}

static void shell_handle_preempt_disable_test(void) {
    scheduler_disable_preempt_test();
    vga_write_line("Timer scheduler test disabled");
}

static void shell_handle_preempt_reset(void) {
    scheduler_reset_preempt_test();
    vga_write_line("Timer scheduler test counters reset");
}

static void shell_handle_task_clear_inactive(void) {
    uint32_t cleared = scheduler_clear_inactive_tasks();

    vga_write("Cleared inactive tasks: ");
    vga_write_u32(cleared);
    vga_write_line("");
}

static void shell_handle_task_once(void) {
    int task_id = scheduler_create_oneshot_kernel_task(
        "shell-oneshot",
        shell_created_noop_task
    );

    if (task_id < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to create one-shot task");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    vga_write("Created one-shot task id: ");
    vga_write_u32((uint32_t)task_id);
    vga_write_line("");
}

static void shell_handle_scheduler_workload_reset(void) {
    scheduler_reset_test_workload_counter();
    shell_created_task_counter = 0;
    shell_yield_workload_counter = 0;
    vga_write_line("Scheduler workload counters reset");
}

static void shell_handle_task_create(const char* args) {
    uint32_t interval_ticks = 0;

    if (!string_to_uint32_auto(args, &interval_ticks) || interval_ticks == 0) {
        vga_write_line("Usage: task-create <positive interval ticks>");
        vga_write_line("Example: task-create 100");
        return;
    }

    int task_id = scheduler_create_kernel_task(
        "shell-noop",
        interval_ticks,
        shell_created_noop_task
    );

    if (task_id < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to create task");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    vga_write("Created task id: ");
    vga_write_u32((uint32_t)task_id);
    vga_write_line("");
}

static void shell_handle_task_disable(const char* args) {
    uint32_t task_id = 0;

    if (!string_to_uint32_auto(args, &task_id) || task_id == 0) {
        vga_write_line("Usage: task-disable <task id>");
        return;
    }

    if (!scheduler_disable_task(task_id)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to disable task");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    vga_write("Disabled task id: ");
    vga_write_u32(task_id);
    vga_write_line("");
}

static void shell_handle_task_sleep(const char* args) {
    char id_text[16];
    uint32_t offset = string_copy_until_space(id_text, args, 16);
    offset = string_skip_spaces(args, offset);

    uint32_t task_id = 0;
    uint32_t sleep_ticks = 0;

    if (
        !string_to_uint32_auto(id_text, &task_id) ||
        !string_to_uint32_auto(args + offset, &sleep_ticks) ||
        task_id == 0 ||
        sleep_ticks == 0
    ) {
        vga_write_line("Usage: task-sleep <task id> <ticks>");
        return;
    }

    if (!scheduler_sleep_task(task_id, sleep_ticks)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to sleep task");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    vga_write("Sleeping task id: ");
    vga_write_u32(task_id);
    vga_write(" for ticks: ");
    vga_write_u32(sleep_ticks);
    vga_write_line("");
}

static void shell_handle_task_wake(const char* args) {
    uint32_t task_id = 0;

    if (!string_to_uint32_auto(args, &task_id) || task_id == 0) {
        vga_write_line("Usage: task-wake <task id>");
        return;
    }

    if (!scheduler_wake_task(task_id)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to wake task");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    vga_write("Woke task id: ");
    vga_write_u32(task_id);
    vga_write_line("");
}

static void shell_handle_task_yield(const char* args) {
    uint32_t task_id = 0;

    if (!string_to_uint32_auto(args, &task_id) || task_id == 0) {
        vga_write_line("Usage: task-yield <task id>");
        return;
    }

    if (!scheduler_yield_task(task_id)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to yield task");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    vga_write("Yielded task id: ");
    vga_write_u32(task_id);
    vga_write_line("");
}

static void shell_print_kernel_status_result(kernel_status_t status) {
    vga_write("Status: ");

    if (status < 0) {
        vga_write("-");
        vga_write_u32((uint32_t)(0 - status));
    } else {
        vga_write_u32((uint32_t)status);
    }

    vga_write(" - ");
    vga_write_line(kernel_status_to_string(status));
}

static void shell_handle_task_sleep_status(const char* args) {
    char id_text[16];
    uint32_t offset = string_copy_until_space(id_text, args, 16);
    offset = string_skip_spaces(args, offset);

    uint32_t task_id = 0;
    uint32_t sleep_ticks = 0;

    if (
        !string_to_uint32_auto(id_text, &task_id) ||
        !string_to_uint32_auto(args + offset, &sleep_ticks)
    ) {
        vga_write_line("Usage: task-sleep-status <task id> <ticks>");
        return;
    }

    kernel_status_t status = scheduler_sleep_task_status(task_id, sleep_ticks);
    shell_print_kernel_status_result(status);
}

static void shell_handle_task_wake_status(const char* args) {
    uint32_t task_id = 0;

    if (!string_to_uint32_auto(args, &task_id)) {
        vga_write_line("Usage: task-wake-status <task id>");
        return;
    }

    kernel_status_t status = scheduler_wake_task_status(task_id);
    shell_print_kernel_status_result(status);
}

static void shell_handle_task_disable_status(const char* args) {
    uint32_t task_id = 0;

    if (!string_to_uint32_auto(args, &task_id)) {
        vga_write_line("Usage: task-disable-status <task id>");
        return;
    }

    kernel_status_t status = scheduler_disable_task_status(task_id);
    shell_print_kernel_status_result(status);
}

static void shell_handle_heap_validate_status(const char* args) {
    uint32_t address = 0;

    if (!string_to_uint32_auto(args, &address)) {
        vga_write_line("Usage: heap-validate-status <address>");
        return;
    }

    kernel_status_t status = heap_validate_pointer_status((void*)address);
    shell_print_kernel_status_result(status);
}

static void shell_handle_vmm_validate_status(const char* args) {
    uint32_t address = 0;

    if (!string_to_uint32_auto(args, &address)) {
        vga_write_line("Usage: vmm-validate-status <address>");
        return;
    }

    kernel_status_t status = vmm_validate_mapping_status(address);
    shell_print_kernel_status_result(status);
}

static void shell_handle_scheduler_workload_create(const char* args) {
    uint32_t interval_ticks = 0;

    if (!string_to_uint32_auto(args, &interval_ticks) || interval_ticks == 0) {
        vga_write_line("Usage: sched-workload-create <positive interval ticks>");
        vga_write_line("Example: sched-workload-create 50");
        return;
    }

    int task_id = scheduler_create_kernel_task(
        "sched-workload",
        interval_ticks,
        shell_yield_workload_task
    );

    if (task_id < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Failed to create scheduler workload");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    vga_write("Created scheduler workload task id: ");
    vga_write_u32((uint32_t)task_id);
    vga_write_line("");
}

static void shell_handle_scheduler_workload_stop(void) {
    scheduler_disable_test_workload();
    vga_write_line("Scheduler built-in workload stopped");
}

static void shell_print_scheduler_workload_status(void) {
    vga_write_line("Scheduler workload status:");

    vga_write("  Built-in workload active: ");
    vga_write_line(scheduler_is_test_workload_active() ? "yes" : "no");

    vga_write("  Built-in workload counter: ");
    vga_write_u32(scheduler_get_test_workload_counter());
    vga_write_line("");

    vga_write("  Shell workload counter: ");
    vga_write_u32(shell_yield_workload_counter);
    vga_write_line("");

    vga_write("  Generic shell task counter: ");
    vga_write_u32(shell_created_task_counter);
    vga_write_line("");
}

static void shell_handle_heap_test(void) {
    vga_write_line("Running heap test...");

    char* small = (char*)kmalloc(32);
    char* medium = (char*)kmalloc(256);
    char* large = (char*)kmalloc(2048);

    if (small == 0 || medium == 0 || large == 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_line("Heap test allocation failed");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        if (small != 0) {
            kfree(small);
        }

        if (medium != 0) {
            kfree(medium);
        }

        if (large != 0) {
            kfree(large);
        }

        return;
    }

    small[0] = 'O';
    small[1] = 'K';
    small[2] = '\0';

    medium[0] = 'M';
    medium[1] = '\0';

    large[0] = 'L';
    large[1] = '\0';

    vga_write("  small allocation: ");
    vga_write_hex_u32((uint32_t)small);
    vga_write(" value=");
    vga_write_line(small);

    vga_write("  medium allocation: ");
    vga_write_hex_u32((uint32_t)medium);
    vga_write(" value=");
    vga_write_line(medium);

    vga_write("  large allocation: ");
    vga_write_hex_u32((uint32_t)large);
    vga_write(" value=");
    vga_write_line(large);

    kfree(large);
    kfree(medium);
    kfree(small);

    vga_write_line("Heap test completed");
}

static void shell_trigger_page_fault(void) {
    volatile uint32_t* invalid_address = (volatile uint32_t*)MEMORY_LAYOUT_VMM_TEST_PAGE;
    volatile uint32_t value = *invalid_address;
    (void)value;
}

static void shell_reboot(void) {
    vga_write_line("Rebooting...");
    outb(0x64, 0xFE);

    for (;;) {
        __asm__ volatile("hlt");
    }
}

static void shell_execute_command(const char* input) {
    shell_command_t command = shell_parse_command(input);

    if (string_equals(command.name, "")) {
        return;
    }

    if (string_equals(command.name, "help")) {
        shell_print_help();
        return;
    }

    if (string_equals(command.name, "about")) {
        shell_print_about();
        return;
    }

    if (string_equals(command.name, "clear")) {
        vga_clear();
        return;
    }

    if (string_equals(command.name, "version")) {
        shell_print_version();
        return;
    }

    if (string_equals(command.name, "sysinfo")) {
        shell_print_sysinfo();
        return;
    }

    if (string_equals(command.name, "mem")) {
        shell_print_memory();
        return;
    }

    if (string_equals(command.name, "heap")) {
        shell_print_heap();
        return;
    }

    if (string_equals(command.name, "heap-audit")) {
        shell_print_heap_audit();
        return;
    }

    if (string_equals(command.name, "heap-validate")) {
        shell_handle_heap_validate(command.args);
        return;
    }

    if (string_equals(command.name, "heap-validate-status")) {
        shell_handle_heap_validate_status(command.args);
        return;
    }

    if (string_equals(command.name, "paging")) {
        shell_print_paging();
        return;
    }

    if (string_equals(command.name, "vmm")) {
        shell_print_vmm();
        return;
    }

    if (string_equals(command.name, "vmm-audit")) {
        shell_print_vmm_audit();
        return;
    }

    if (string_equals(command.name, "vmm-validate-status")) {
        shell_handle_vmm_validate_status(command.args);
        return;
    }

    if (string_equals(command.name, "layout")) {
        shell_print_layout();
        return;
    }

    if (string_equals(command.name, "virt")) {
        shell_handle_virt(command.args);
        return;
    }

    if (string_equals(command.name, "map-info")) {
        shell_handle_virt(command.args);
        return;
    }

    if (string_equals(command.name, "scheduler")) {
        shell_print_scheduler();
        return;
    }

    if (string_equals(command.name, "policy")) {
        shell_print_scheduler_policy();
        return;
    }

    if (string_equals(command.name, "readyq")) {
        shell_print_ready_queue();
        return;
    }

    if (string_equals(command.name, "scheduler-audit")) {
        uint32_t issues = scheduler_audit();

        if (issues == 0) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_write_line("Scheduler audit passed");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        } else {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_write("Scheduler audit found issues: ");
            vga_write_u32(issues);
            vga_write_line("");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }

        shell_print_scheduler_audit();
        return;
    }

    if (string_equals(command.name, "scheduler-repair")) {
        uint32_t repaired = scheduler_repair();

        vga_write("Scheduler repaired issues: ");
        vga_write_u32(repaired);
        vga_write_line("");

        shell_print_scheduler_audit();
        return;
    }

    if (string_equals(command.name, "scheduler-lock-status")) {
        shell_print_scheduler_lock_status();
        return;
    }

    if (string_equals(command.name, "assert")) {
        kernel_assert_print_stats();
        return;
    }

    if (string_equals(command.name, "assert-pass")) {
        kernel_assert_message(1 == 1, "assert-pass command should not fail");
        vga_write_line("Assertion pass check completed");
        return;
    }

    if (string_equals(command.name, "assert-fail")) {
        kernel_assert_message(0, "controlled assertion failure from shell");
        return;
    }

    if (string_equals(command.name, "status")) {
        shell_print_status_table();
        return;
    }

    if (string_equals(command.name, "status-is")) {
        shell_handle_status_is(command.args);
        return;
    }

    if (string_equals(command.name, "scheduler-lock-test")) {
        if (scheduler_run_lock_test()) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_write_line("Scheduler lock test passed");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        } else {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_write_line("Scheduler lock test failed");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }

        shell_print_scheduler_lock_status();
        return;
    }

    if (string_equals(command.name, "sched-workload-create")) {
        shell_handle_scheduler_workload_create(command.args);
        return;
    }

    if (string_equals(command.name, "sched-workload-stop")) {
        shell_handle_scheduler_workload_stop();
        return;
    }

    if (string_equals(command.name, "sched-workload-reset")) {
        shell_handle_scheduler_workload_reset();
        return;
    }

    if (string_equals(command.name, "sched-workload-status")) {
        shell_print_scheduler_workload_status();
        return;
    }

    if (string_equals(command.name, "tasks")) {
        shell_print_tasks();
        return;
    }

    if (string_equals(command.name, "tss-status")) {
        shell_print_tss_status();
        return;
    }

    if (string_equals(command.name, "ring3-status")) {
        shell_print_ring3_status();
        return;
    }

    if (string_equals(command.name, "syscalls")) {
        shell_print_syscall_status();
        return;
    }

    if (string_equals(command.name, "fd-status")) {
        shell_print_fd_status();
        return;
    }

    if (string_equals(command.name, "images")) {
        shell_print_user_images();
        return;
    }

    if (string_equals(command.name, "exec-status")) {
        shell_print_exec_status();
        return;
    }

    if (string_equals(command.name, "fs-status")) {
        shell_print_fs_status();
        return;
    }

    if (string_equals(command.name, "loader-status")) {
        shell_print_loader_status();
        return;
    }

    if (string_equals(command.name, "elf-status")) {
        shell_print_elf_status();
        return;
    }

    if (string_equals(command.name, "elf-check")) {
        shell_handle_elf_check(command.args);
        return;
    }

    if (string_equals(command.name, "ls")) {
        shell_handle_ls(command.args);
        return;
    }

    if (string_equals(command.name, "cat")) {
        shell_handle_cat(command.args);
        return;
    }

    if (string_equals(command.name, "stat")) {
        shell_handle_stat(command.args);
        return;
    }

    if (string_equals(command.name, "ps")) {
        shell_print_processes();
        return;
    }

    if (string_equals(command.name, "proc-status")) {
        shell_print_process_status();
        return;
    }

    if (string_equals(command.name, "proc-info")) {
        shell_handle_proc_info(command.args);
        return;
    }

    if (string_equals(command.name, "proc-clear-exited")) {
        shell_handle_proc_clear_exited();
        return;
    }

    if (string_equals(command.name, "wait-last")) {
        shell_handle_wait_last();
        return;
    }

    if (string_equals(command.name, "run-ready")) {
        shell_handle_run_ready();
        return;
    }

    if (string_equals(command.name, "run-pid")) {
        shell_handle_run_pid(command.args);
        return;
    }

    if (string_equals(command.name, "exec")) {
        shell_handle_exec(command.args);
        return;
    }

    if (string_equals(command.name, "init")) {
        shell_start_init();
        return;
    }

    if (string_equals(command.name, "context")) {
        shell_print_context();
        return;
    }

    if (string_equals(command.name, "ctx-test")) {
        shell_print_context();
        return;
    }

    if (string_equals(command.name, "ctx-self-test")) {
        if (scheduler_run_context_self_test()) {
            vga_write_line("Context self-test passed");
        } else {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_write_line("Context self-test failed");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }
        return;
    }

    if (string_equals(command.name, "switch-test")) {
        if (scheduler_run_switch_test()) {
            vga_write_line("Context switch test passed");
        } else {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_write_line("Context switch test failed");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }
        return;
    }

    if (string_equals(command.name, "switch-status")) {
        shell_print_switch_status();
        return;
    }

    if (string_equals(command.name, "switch-reset")) {
        scheduler_reset_switch_test();
        vga_write_line("Context switch test state reset");
        return;
    }

    if (string_equals(command.name, "preempt-status")) {
        shell_print_preempt_status();
        return;
    }

    if (string_equals(command.name, "preempt-enable-test")) {
        shell_handle_preempt_enable_test(command.args);
        return;
    }

    if (string_equals(command.name, "preempt-disable-test")) {
        shell_handle_preempt_disable_test();
        return;
    }

    if (string_equals(command.name, "preempt-reset")) {
        shell_handle_preempt_reset();
        return;
    }

    if (string_equals(command.name, "task-create")) {
        shell_handle_task_create(command.args);
        return;
    }

    if (string_equals(command.name, "task-once")) {
        shell_handle_task_once();
        return;
    }

    if (string_equals(command.name, "task-info")) {
        shell_handle_task_info(command.args);
        return;
    }

    if (string_equals(command.name, "task-run-now")) {
        shell_handle_task_run_now(command.args);
        return;
    }

    if (string_equals(command.name, "task-switch-run")) {
        shell_handle_task_switch_run(command.args);
        return;
    }

    if (string_equals(command.name, "task-sleep")) {
        shell_handle_task_sleep(command.args);
        return;
    }

    if (string_equals(command.name, "task-sleep-status")) {
        shell_handle_task_sleep_status(command.args);
        return;
    }

    if (string_equals(command.name, "task-wake")) {
        shell_handle_task_wake(command.args);
        return;
    }

    if (string_equals(command.name, "task-wake-status")) {
        shell_handle_task_wake_status(command.args);
        return;
    }

    if (string_equals(command.name, "task-yield")) {
        shell_handle_task_yield(command.args);
        return;
    }

    if (string_equals(command.name, "task-current")) {
        shell_print_current_task();
        return;
    }

    if (string_equals(command.name, "task-disable")) {
        shell_handle_task_disable(command.args);
        return;
    }

    if (string_equals(command.name, "task-disable-status")) {
        shell_handle_task_disable_status(command.args);
        return;
    }

    if (string_equals(command.name, "task-clear")) {
        shell_handle_task_clear_inactive();
        return;
    }

    if (string_equals(command.name, "pf-test")) {
        vga_write_line("Triggering controlled page fault at 0x08000000...");
        shell_trigger_page_fault();
        return;
    }

    if (string_equals(command.name, "vmap-test")) {
        shell_handle_vmap_test();
        return;
    }

    if (string_equals(command.name, "vunmap-test")) {
        shell_handle_vunmap_test();
        return;
    }


    if (string_equals(command.name, "ticks")) {
        shell_print_ticks();
        return;
    }

    if (string_equals(command.name, "uptime")) {
        shell_print_uptime();
        return;
    }

    if (string_equals(command.name, "echo")) {
        vga_write_line(command.args);
        return;
    }

    if (string_equals(command.name, "color")) {
        shell_handle_color(command.args);
        return;
    }

    if (string_equals(command.name, "alloc")) {
        shell_handle_alloc(command.args);
        return;
    }

    if (string_equals(command.name, "pmm-list")) {
        shell_print_pmm_list();
        return;
    }

    if (string_equals(command.name, "free-last")) {
        shell_handle_free_last();
        return;
    }

    if (string_equals(command.name, "pmm-free-all")) {
        shell_handle_pmm_free_all();
        return;
    }

    if (string_equals(command.name, "kmalloc")) {
        shell_handle_kmalloc(command.args);
        return;
    }

    if (string_equals(command.name, "heap-list")) {
        shell_print_heap_list();
        return;
    }

    if (string_equals(command.name, "kfree-last")) {
        shell_handle_kfree_last();
        return;
    }

    if (string_equals(command.name, "heap-free-all")) {
        shell_handle_heap_free_all();
        return;
    }

    if (string_equals(command.name, "heap-test")) {
        shell_handle_heap_test();
        return;
    }

    if (string_equals(command.name, "reboot")) {
        shell_reboot();
        return;
    }

    if (string_equals(command.name, "panic")) {
        kernel_panic("Manual shell panic command");
        return;
    }

    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write("Unknown command: ");
    vga_write_line(command.name);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

void shell_initialize(void) {
    shell_clear_input();
    shell_clear_tracking_tables();

    vga_write_line("");
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_line("Liam OS shell ready. Type 'help' for commands.");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    shell_print_prompt();
}

void shell_handle_character(char character) {
    if (character == '\n') {
        vga_write_line("");

        input_buffer[input_length] = '\0';
        shell_execute_command(input_buffer);
        shell_clear_input();
        shell_print_prompt();
        return;
    }

    if (character == '\b') {
        if (input_length > 0) {
            input_length--;
            input_buffer[input_length] = '\0';
            vga_backspace();
        }

        return;
    }

    if (input_length >= SHELL_INPUT_MAX - 1) {
        return;
    }

    input_buffer[input_length] = character;
    input_length++;

    vga_write_char(character);
}