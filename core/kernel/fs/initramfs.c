#include "initramfs.h"
#include "../core/version.h"

extern uint8_t ring3_user_image_start[];
extern uint8_t ring3_user_image_end[];
extern uint8_t ring3_hello_image_start[];
extern uint8_t ring3_hello_image_end[];
extern uint8_t ring3_syscheck_image_start[];
extern uint8_t ring3_syscheck_image_end[];
extern uint8_t ring3_pid_image_start[];
extern uint8_t ring3_pid_image_end[];
extern uint8_t ring3_ticks_image_start[];
extern uint8_t ring3_ticks_image_end[];
extern uint8_t ring3_about_image_start[];
extern uint8_t ring3_about_image_end[];
extern uint8_t ring3_os_release_image_start[];
extern uint8_t ring3_os_release_image_end[];
extern uint8_t ring3_cat_image_start[];
extern uint8_t ring3_cat_image_end[];
extern uint8_t ring3_clear_image_start[];
extern uint8_t ring3_clear_image_end[];
extern uint8_t ring3_help_image_start[];
extern uint8_t ring3_help_image_end[];
extern uint8_t ring3_args_image_start[];
extern uint8_t ring3_args_image_end[];
extern uint8_t ring3_echo_image_start[];
extern uint8_t ring3_echo_image_end[];
extern uint8_t ring3_sh_image_start[];
extern uint8_t ring3_sh_image_end[];
extern uint8_t ring3_uptime_image_start[];
extern uint8_t ring3_uptime_image_end[];

static const uint8_t os_release_data[] =
    "NAME=\"Liam OS\"\n"
    "VERSION=\"" LIAM_OS_VERSION "\"\n"
    "CODENAME=\"" LIAM_OS_CODENAME "\"\n"
    "ARCH=\"" LIAM_OS_ARCH "\"\n";

static const uint8_t console_device_data[] =
    "console device\n";

static const uint8_t elf32_sample_data[] = {
    0x7F, 'E', 'L', 'F',
    0x01, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x02, 0x00,
    0x03, 0x00,
    0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x10,
    0x34, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x34, 0x00,
    0x20, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00
};

static const uint8_t hello_elf_data[] = {
    0x7F, 'E', 'L', 'F',
    0x01, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x02, 0x00,
    0x03, 0x00,
    0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10,
    0x34, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x34, 0x00,
    0x20, 0x00,
    0x01, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,

    0x01, 0x00, 0x00, 0x00,
    0x54, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x10,
    0x5C, 0x00, 0x00, 0x00,
    0x5C, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x00, 0x00,

    0xE8, 0x00, 0x00, 0x00, 0x00,
    0x5E,
    0x89, 0xF3,
    0x83, 0xC3, 0x2F,
    0xB9, 0x28, 0x00, 0x00, 0x00,
    0xBA, 0x00, 0x00, 0x00, 0x00,
    0xB8, 0x02, 0x00, 0x00, 0x00,
    0xCD, 0x80,
    0xBB, 0x00, 0x00, 0x00, 0x00,
    0xB9, 0x00, 0x00, 0x00, 0x00,
    0xBA, 0x00, 0x00, 0x00, 0x00,
    0xB8, 0x01, 0x00, 0x00, 0x00,
    0xCD, 0x80,
    0x0F, 0x0B,

    'h', 'e', 'l', 'l', 'o', ':', ' ', 'E',
    'L', 'F', '3', '2', ' ', 'u', 's', 'e',
    'r', 's', 'p', 'a', 'c', 'e', ' ', 'p',
    'r', 'o', 'g', 'r', 'a', 'm', ' ', 'e',
    'x', 'e', 'c', 'u', 't', 'e', 'd', '\n'
};

static vfs_node_t root_node;
static vfs_node_t sbin_node;
static vfs_node_t bin_node;
static vfs_node_t etc_node;
static vfs_node_t dev_node;
static vfs_node_t tests_node;
static vfs_node_t init_node;
static vfs_node_t hello_node;
static vfs_node_t hello_elf_node;
static vfs_node_t syscheck_node;
static vfs_node_t pid_node;
static vfs_node_t ticks_node;
static vfs_node_t about_node;
static vfs_node_t os_release_program_node;
static vfs_node_t cat_node;
static vfs_node_t clear_node;
static vfs_node_t help_node;
static vfs_node_t uptime_node;
static vfs_node_t args_node;
static vfs_node_t echo_node;
static vfs_node_t sh_node;
static vfs_node_t os_release_node;
static vfs_node_t console_node;
static vfs_node_t elf32_sample_node;

static const vfs_node_t* root_children[] = {
    &sbin_node,
    &bin_node,
    &etc_node,
    &dev_node,
    &tests_node
};

static const vfs_node_t* sbin_children[] = {
    &init_node
};

static const vfs_node_t* bin_children[] = {
    &hello_node,
    &hello_elf_node,
    &syscheck_node,
    &pid_node,
    &ticks_node,
    &about_node,
    &os_release_program_node,
    &cat_node,
    &clear_node,
    &help_node,
    &uptime_node,
    &args_node,
    &echo_node,
    &sh_node
};

static const vfs_node_t* etc_children[] = {
    &os_release_node
};

static const vfs_node_t* dev_children[] = {
    &console_node
};

static const vfs_node_t* tests_children[] = {
    &elf32_sample_node
};

static initramfs_info_t initramfs_info;

static void initramfs_define_nodes(void)
{
    root_node.name = "";
    root_node.path = "/";
    root_node.type = VFS_NODE_DIRECTORY;
    root_node.data = 0;
    root_node.size = 0;
    root_node.mode = 0755;
    root_node.flags = 0;
    root_node.parent = 0;
    root_node.children = root_children;
    root_node.child_count = sizeof(root_children) / sizeof(root_children[0]);

    sbin_node.name = "sbin";
    sbin_node.path = "/sbin";
    sbin_node.type = VFS_NODE_DIRECTORY;
    sbin_node.data = 0;
    sbin_node.size = 0;
    sbin_node.mode = 0755;
    sbin_node.flags = 0;
    sbin_node.parent = &root_node;
    sbin_node.children = sbin_children;
    sbin_node.child_count = sizeof(sbin_children) / sizeof(sbin_children[0]);

    bin_node.name = "bin";
    bin_node.path = "/bin";
    bin_node.type = VFS_NODE_DIRECTORY;
    bin_node.data = 0;
    bin_node.size = 0;
    bin_node.mode = 0755;
    bin_node.flags = 0;
    bin_node.parent = &root_node;
    bin_node.children = bin_children;
    bin_node.child_count = sizeof(bin_children) / sizeof(bin_children[0]);

    etc_node.name = "etc";
    etc_node.path = "/etc";
    etc_node.type = VFS_NODE_DIRECTORY;
    etc_node.data = 0;
    etc_node.size = 0;
    etc_node.mode = 0755;
    etc_node.flags = 0;
    etc_node.parent = &root_node;
    etc_node.children = etc_children;
    etc_node.child_count = sizeof(etc_children) / sizeof(etc_children[0]);

    dev_node.name = "dev";
    dev_node.path = "/dev";
    dev_node.type = VFS_NODE_DIRECTORY;
    dev_node.data = 0;
    dev_node.size = 0;
    dev_node.mode = 0755;
    dev_node.flags = 0;
    dev_node.parent = &root_node;
    dev_node.children = dev_children;
    dev_node.child_count = sizeof(dev_children) / sizeof(dev_children[0]);

    tests_node.name = "tests";
    tests_node.path = "/tests";
    tests_node.type = VFS_NODE_DIRECTORY;
    tests_node.data = 0;
    tests_node.size = 0;
    tests_node.mode = 0755;
    tests_node.flags = 0;
    tests_node.parent = &root_node;
    tests_node.children = tests_children;
    tests_node.child_count = sizeof(tests_children) / sizeof(tests_children[0]);

    init_node.name = "init";
    init_node.path = "/sbin/init";
    init_node.type = VFS_NODE_FILE;
    init_node.data = ring3_user_image_start;
    init_node.size = (uint32_t)(ring3_user_image_end - ring3_user_image_start);
    init_node.mode = 0755;
    init_node.flags = 0;
    init_node.parent = &sbin_node;
    init_node.children = 0;
    init_node.child_count = 0;

    hello_node.name = "hello";
    hello_node.path = "/bin/hello";
    hello_node.type = VFS_NODE_FILE;
    hello_node.data = ring3_hello_image_start;
    hello_node.size = (uint32_t)(ring3_hello_image_end - ring3_hello_image_start);
    hello_node.mode = 0755;
    hello_node.flags = 0;
    hello_node.parent = &bin_node;
    hello_node.children = 0;
    hello_node.child_count = 0;

    hello_elf_node.name = "hello-elf";
    hello_elf_node.path = "/bin/hello-elf";
    hello_elf_node.type = VFS_NODE_FILE;
    hello_elf_node.data = hello_elf_data;
    hello_elf_node.size = sizeof(hello_elf_data);
    hello_elf_node.mode = 0755;
    hello_elf_node.flags = 0;
    hello_elf_node.parent = &bin_node;
    hello_elf_node.children = 0;
    hello_elf_node.child_count = 0;

    syscheck_node.name = "syscheck";
    syscheck_node.path = "/bin/syscheck";
    syscheck_node.type = VFS_NODE_FILE;
    syscheck_node.data = ring3_syscheck_image_start;
    syscheck_node.size = (uint32_t)(ring3_syscheck_image_end - ring3_syscheck_image_start);
    syscheck_node.mode = 0755;
    syscheck_node.flags = 0;
    syscheck_node.parent = &bin_node;
    syscheck_node.children = 0;
    syscheck_node.child_count = 0;

    pid_node.name = "pid";
    pid_node.path = "/bin/pid";
    pid_node.type = VFS_NODE_FILE;
    pid_node.data = ring3_pid_image_start;
    pid_node.size = (uint32_t)(ring3_pid_image_end - ring3_pid_image_start);
    pid_node.mode = 0755;
    pid_node.flags = 0;
    pid_node.parent = &bin_node;
    pid_node.children = 0;
    pid_node.child_count = 0;

    ticks_node.name = "ticks";
    ticks_node.path = "/bin/ticks";
    ticks_node.type = VFS_NODE_FILE;
    ticks_node.data = ring3_ticks_image_start;
    ticks_node.size = (uint32_t)(ring3_ticks_image_end - ring3_ticks_image_start);
    ticks_node.mode = 0755;
    ticks_node.flags = 0;
    ticks_node.parent = &bin_node;
    ticks_node.children = 0;
    ticks_node.child_count = 0;

    about_node.name = "about";
    about_node.path = "/bin/about";
    about_node.type = VFS_NODE_FILE;
    about_node.data = ring3_about_image_start;
    about_node.size = (uint32_t)(ring3_about_image_end - ring3_about_image_start);
    about_node.mode = 0755;
    about_node.flags = 0;
    about_node.parent = &bin_node;
    about_node.children = 0;
    about_node.child_count = 0;

    os_release_program_node.name = "os-release";
    os_release_program_node.path = "/bin/os-release";
    os_release_program_node.type = VFS_NODE_FILE;
    os_release_program_node.data = ring3_os_release_image_start;
    os_release_program_node.size = (uint32_t)(ring3_os_release_image_end - ring3_os_release_image_start);
    os_release_program_node.mode = 0755;
    os_release_program_node.flags = 0;
    os_release_program_node.parent = &bin_node;
    os_release_program_node.children = 0;
    os_release_program_node.child_count = 0;

    cat_node.name = "cat";
    cat_node.path = "/bin/cat";
    cat_node.type = VFS_NODE_FILE;
    cat_node.data = ring3_cat_image_start;
    cat_node.size = (uint32_t)(ring3_cat_image_end - ring3_cat_image_start);
    cat_node.mode = 0755;
    cat_node.flags = 0;
    cat_node.parent = &bin_node;
    cat_node.children = 0;
    cat_node.child_count = 0;

    clear_node.name = "clear";
    clear_node.path = "/bin/clear";
    clear_node.type = VFS_NODE_FILE;
    clear_node.data = ring3_clear_image_start;
    clear_node.size = (uint32_t)(ring3_clear_image_end - ring3_clear_image_start);
    clear_node.mode = 0755;
    clear_node.flags = 0;
    clear_node.parent = &bin_node;
    clear_node.children = 0;
    clear_node.child_count = 0;

    help_node.name = "help";
    help_node.path = "/bin/help";
    help_node.type = VFS_NODE_FILE;
    help_node.data = ring3_help_image_start;
    help_node.size = (uint32_t)(ring3_help_image_end - ring3_help_image_start);
    help_node.mode = 0755;
    help_node.flags = 0;
    help_node.parent = &bin_node;
    help_node.children = 0;
    help_node.child_count = 0;

    uptime_node.name = "uptime";
    uptime_node.path = "/bin/uptime";
    uptime_node.type = VFS_NODE_FILE;
    uptime_node.data = ring3_uptime_image_start;
    uptime_node.size = (uint32_t)(ring3_uptime_image_end - ring3_uptime_image_start);
    uptime_node.mode = 0755;
    uptime_node.flags = 0;
    uptime_node.parent = &bin_node;
    uptime_node.children = 0;
    uptime_node.child_count = 0;

    args_node.name = "args";
    args_node.path = "/bin/args";
    args_node.type = VFS_NODE_FILE;
    args_node.data = ring3_args_image_start;
    args_node.size = (uint32_t)(ring3_args_image_end - ring3_args_image_start);
    args_node.mode = 0755;
    args_node.flags = 0;
    args_node.parent = &bin_node;
    args_node.children = 0;
    args_node.child_count = 0;

    echo_node.name = "echo";
    echo_node.path = "/bin/echo";
    echo_node.type = VFS_NODE_FILE;
    echo_node.data = ring3_echo_image_start;
    echo_node.size = (uint32_t)(ring3_echo_image_end - ring3_echo_image_start);
    echo_node.mode = 0755;
    echo_node.flags = 0;
    echo_node.parent = &bin_node;
    echo_node.children = 0;
    echo_node.child_count = 0;

    sh_node.name = "sh";
    sh_node.path = "/bin/sh";
    sh_node.type = VFS_NODE_FILE;
    sh_node.data = ring3_sh_image_start;
    sh_node.size = (uint32_t)(ring3_sh_image_end - ring3_sh_image_start);
    sh_node.mode = 0755;
    sh_node.flags = 0;
    sh_node.parent = &bin_node;
    sh_node.children = 0;
    sh_node.child_count = 0;

    os_release_node.name = "os-release";
    os_release_node.path = "/etc/os-release";
    os_release_node.type = VFS_NODE_FILE;
    os_release_node.data = os_release_data;
    os_release_node.size = sizeof(os_release_data) - 1;
    os_release_node.mode = 0644;
    os_release_node.flags = 0;
    os_release_node.parent = &etc_node;
    os_release_node.children = 0;
    os_release_node.child_count = 0;

    console_node.name = "console";
    console_node.path = "/dev/console";
    console_node.type = VFS_NODE_DEVICE;
    console_node.data = console_device_data;
    console_node.size = sizeof(console_device_data) - 1;
    console_node.mode = 0600;
    console_node.flags = 0;
    console_node.parent = &dev_node;
    console_node.children = 0;
    console_node.child_count = 0;

    elf32_sample_node.name = "elf32-sample";
    elf32_sample_node.path = "/tests/elf32-sample";
    elf32_sample_node.type = VFS_NODE_FILE;
    elf32_sample_node.data = elf32_sample_data;
    elf32_sample_node.size = sizeof(elf32_sample_data);
    elf32_sample_node.mode = 0644;
    elf32_sample_node.flags = 0;
    elf32_sample_node.parent = &tests_node;
    elf32_sample_node.children = 0;
    elf32_sample_node.child_count = 0;
}

void initramfs_initialize(void)
{
    initramfs_define_nodes();

    initramfs_info.initialized = 1;
    initramfs_info.last_status = KERNEL_OK;
    initramfs_info.node_count = 24;
    initramfs_info.mount_count = 0;
}

kernel_status_t initramfs_mount(void)
{
    if (!initramfs_info.initialized)
    {
        initramfs_info.last_status = KERNEL_ERROR_NOT_INITIALIZED;
        return initramfs_info.last_status;
    }

    kernel_status_t status = vfs_mount_static_tree(&root_node);
    initramfs_info.last_status = status;

    if (status == KERNEL_OK)
    {
        initramfs_info.mount_count++;
    }

    return status;
}

const initramfs_info_t* initramfs_get_info(void)
{
    return &initramfs_info;
}
