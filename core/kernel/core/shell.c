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
static shell_pmm_allocation_t pmm_allocations[SHELL_MAX_TRACKED_PMM_ALLOCATIONS];
static shell_heap_allocation_t heap_allocations[SHELL_MAX_TRACKED_HEAP_ALLOCATIONS];

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
    vga_write_line("  run-all-ready    Run all READY userspace processes");
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

                if (string_equals(command.name, "wait-last")) {
        shell_handle_wait_last();
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

                                                                                                            
            if (string_equals(command.name, "echo")) {
        vga_write_line(command.args);
        return;
    }

                                            if (string_equals(command.name, "reboot")) {
        shell_reboot();
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