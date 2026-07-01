#include "vfs.h"

extern const u8 x86_64_user_eventd_image_start[];
extern const u8 x86_64_user_eventd_image_end[];
extern const u8 x86_64_user_datatest_image_start[];
extern const u8 x86_64_user_datatest_image_end[];
extern const u8 x86_64_user_file_manager_image_start[];
extern const u8 x86_64_user_file_manager_image_end[];
extern const u8 x86_64_user_hello_image_start[];
extern const u8 x86_64_user_hello_image_end[];
extern const u8 x86_64_user_sysinfo_image_start[];
extern const u8 x86_64_user_sysinfo_image_end[];
extern const u8 x86_64_user_inputd_image_start[];
extern const u8 x86_64_user_inputd_image_end[];
extern const u8 x86_64_user_sessiond_image_start[];
extern const u8 x86_64_user_sessiond_image_end[];
extern const u8 x86_64_user_settings_image_start[];
extern const u8 x86_64_user_settings_image_end[];
extern const u8 x86_64_user_storaged_image_start[];
extern const u8 x86_64_user_storaged_image_end[];
extern const u8 x86_64_user_system_monitor_image_start[];
extern const u8 x86_64_user_system_monitor_image_end[];
extern const u8 x86_64_user_terminal_image_start[];
extern const u8 x86_64_user_terminal_image_end[];
extern const u8 x86_64_user_text_editor_image_start[];
extern const u8 x86_64_user_text_editor_image_end[];
extern const u8 x86_64_user_timed_image_start[];
extern const u8 x86_64_user_timed_image_end[];
extern const u8 x86_64_user_windowd_image_start[];
extern const u8 x86_64_user_windowd_image_end[];
extern const u8 x86_64_user_windowd_service_image_start[];
extern const u8 x86_64_user_windowd_service_image_end[];

#define X86_64_VFS_RAM_NODE_BASE 0x80000000U
#define X86_64_VFS_DIR_LIST_BYTES 512ULL

struct x86_64_vfs_node {
    const char *path;
    const u8 *data;
    u64 size;
    u32 type;
};

struct x86_64_vfs_ram_node {
    u32 used;
    u32 type;
    char path[X86_64_VFS_PATH_MAX_BYTES];
    u8 data[X86_64_VFS_RAM_FILE_BYTES];
    u64 size;
};

static const char x86_64_vfs_dir_root[] =
    "bin\n"
    "etc\n"
    "proc\n"
    "sbin\n"
    "tmp\n"
    "usr\n";

static const char x86_64_vfs_dir_bin[] =
    "datatest\n"
    "eventd\n"
    "file-manager\n"
    "hello\n"
    "inputd\n"
    "sessiond\n"
    "settings\n"
    "storaged\n"
    "sysinfo\n"
    "system-monitor\n"
    "terminal\n"
    "text-editor\n"
    "timed\n"
    "windowd\n"
    "windowd-service\n";
static const char x86_64_vfs_dir_sbin[] = "";
static const char x86_64_vfs_dir_etc[] =
    "desktop.conf\n"
    "motd\n"
    "os-release\n"
    "storage.conf\n";
static const char x86_64_vfs_dir_proc[] = "version\n";
static const char x86_64_vfs_dir_usr[] = "share\n";
static const char x86_64_vfs_dir_usr_share[] =
    "files.txt\n"
    "help.txt\n";

static const char x86_64_vfs_file_os_release[] =
    "NAME=Liam_OS\n"
    "ARCH=x86_64\n"
    "BUILD=dev\n";

static const char x86_64_vfs_file_motd[] =
    "Liam_OS x86_64 RAM filesystem mounted.\n";

static const char x86_64_vfs_file_version[] =
    "Liam_OS Core x86_64 0.8.69-dev\n";

static const char x86_64_vfs_file_desktop_config[] =
    "desktop.session=/bin/sessiond\n"
    "desktop.compositor=/bin/windowd-service\n"
    "desktop.login=planned\n";

static const char x86_64_vfs_file_storage_config[] =
    "storage.ramfs=enabled\n"
    "storage.persistent=planned\n";

static const char x86_64_vfs_file_help[] =
    "Available commands:\n"
    "help, about, version, pid, ps, deskcheck, wait, echo, ls, cat, stat, exec, clear, exit\n"
    "Executable files:\n"
    "/bin/datatest\n"
    "/bin/eventd\n"
    "/bin/file-manager\n"
    "/bin/hello\n"
    "/bin/inputd\n"
    "/bin/sessiond\n"
    "/bin/settings\n"
    "/bin/storaged\n"
    "/bin/sysinfo\n"
    "/bin/system-monitor\n"
    "/bin/terminal\n"
    "/bin/text-editor\n"
    "/bin/timed\n"
    "/bin/windowd\n"
    "/bin/windowd-service\n";

static const char x86_64_vfs_file_files[] =
    "/\n"
    "/bin\n"
    "/bin/datatest\n"
    "/bin/eventd\n"
    "/bin/file-manager\n"
    "/bin/hello\n"
    "/bin/inputd\n"
    "/bin/sessiond\n"
    "/bin/settings\n"
    "/bin/storaged\n"
    "/bin/sysinfo\n"
    "/bin/system-monitor\n"
    "/bin/terminal\n"
    "/bin/text-editor\n"
    "/bin/timed\n"
    "/bin/windowd\n"
    "/bin/windowd-service\n"
    "/etc\n"
    "/etc/desktop.conf\n"
    "/etc/motd\n"
    "/etc/os-release\n"
    "/etc/storage.conf\n"
    "/proc\n"
    "/proc/version\n"
    "/sbin\n"
    "/tmp\n"
    "/tmp/session.txt\n"
    "/usr\n"
    "/usr/share\n"
    "/usr/share/files.txt\n"
    "/usr/share/help.txt\n";

static const struct x86_64_vfs_node x86_64_vfs_nodes[] = {
    {"/", (const u8 *)x86_64_vfs_dir_root, sizeof(x86_64_vfs_dir_root) - 1ULL, X86_64_VFS_NODE_DIRECTORY},
    {"/bin", (const u8 *)x86_64_vfs_dir_bin, sizeof(x86_64_vfs_dir_bin) - 1ULL, X86_64_VFS_NODE_DIRECTORY},
    {"/etc", (const u8 *)x86_64_vfs_dir_etc, sizeof(x86_64_vfs_dir_etc) - 1ULL, X86_64_VFS_NODE_DIRECTORY},
    {"/proc", (const u8 *)x86_64_vfs_dir_proc, sizeof(x86_64_vfs_dir_proc) - 1ULL, X86_64_VFS_NODE_DIRECTORY},
    {"/sbin", (const u8 *)x86_64_vfs_dir_sbin, sizeof(x86_64_vfs_dir_sbin) - 1ULL, X86_64_VFS_NODE_DIRECTORY},
    {"/tmp", (const u8 *)0, 0ULL, X86_64_VFS_NODE_DIRECTORY},
    {"/usr", (const u8 *)x86_64_vfs_dir_usr, sizeof(x86_64_vfs_dir_usr) - 1ULL, X86_64_VFS_NODE_DIRECTORY},
    {"/usr/share", (const u8 *)x86_64_vfs_dir_usr_share, sizeof(x86_64_vfs_dir_usr_share) - 1ULL, X86_64_VFS_NODE_DIRECTORY},
    {"/bin/datatest", x86_64_user_datatest_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/eventd", x86_64_user_eventd_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/file-manager", x86_64_user_file_manager_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/hello", x86_64_user_hello_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/inputd", x86_64_user_inputd_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/sessiond", x86_64_user_sessiond_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/settings", x86_64_user_settings_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/storaged", x86_64_user_storaged_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/sysinfo", x86_64_user_sysinfo_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/system-monitor", x86_64_user_system_monitor_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/terminal", x86_64_user_terminal_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/text-editor", x86_64_user_text_editor_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/timed", x86_64_user_timed_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/windowd", x86_64_user_windowd_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/windowd-service", x86_64_user_windowd_service_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/etc/desktop.conf", (const u8 *)x86_64_vfs_file_desktop_config, sizeof(x86_64_vfs_file_desktop_config) - 1ULL, X86_64_VFS_NODE_FILE},
    {"/etc/motd", (const u8 *)x86_64_vfs_file_motd, sizeof(x86_64_vfs_file_motd) - 1ULL, X86_64_VFS_NODE_FILE},
    {"/etc/os-release", (const u8 *)x86_64_vfs_file_os_release, sizeof(x86_64_vfs_file_os_release) - 1ULL, X86_64_VFS_NODE_FILE},
    {"/etc/storage.conf", (const u8 *)x86_64_vfs_file_storage_config, sizeof(x86_64_vfs_file_storage_config) - 1ULL, X86_64_VFS_NODE_FILE},
    {"/proc/version", (const u8 *)x86_64_vfs_file_version, sizeof(x86_64_vfs_file_version) - 1ULL, X86_64_VFS_NODE_FILE},
    {"/tmp/session.txt", (const u8 *)0, 0ULL, X86_64_VFS_NODE_FILE},
    {"/usr/share/files.txt", (const u8 *)x86_64_vfs_file_files, sizeof(x86_64_vfs_file_files) - 1ULL, X86_64_VFS_NODE_FILE},
    {"/usr/share/help.txt", (const u8 *)x86_64_vfs_file_help, sizeof(x86_64_vfs_file_help) - 1ULL, X86_64_VFS_NODE_FILE},
};

#define X86_64_VFS_STATIC_NODE_COUNT ((u32)(sizeof(x86_64_vfs_nodes) / sizeof(x86_64_vfs_nodes[0])))

static u8 x86_64_vfs_session_storage[X86_64_VFS_RAM_FILE_BYTES];
static u64 x86_64_vfs_session_storage_size;
static struct x86_64_vfs_ram_node x86_64_vfs_ram_nodes[X86_64_VFS_RAM_NODE_COUNT];
static char x86_64_vfs_dir_listing[X86_64_VFS_DIR_LIST_BYTES];

static u32 x86_64_vfs_is_ram_index(u32 index)
{
    return (index >= X86_64_VFS_RAM_NODE_BASE &&
            (index - X86_64_VFS_RAM_NODE_BASE) < X86_64_VFS_RAM_NODE_COUNT) ? 1U : 0U;
}

static u32 x86_64_vfs_path_equals(const char *path, const char *kernel_path)
{
    for (u64 i = 0ULL; i < X86_64_VFS_PATH_MAX_BYTES; ++i) {
        if (path[i] != kernel_path[i]) {
            return 0U;
        }
        if (path[i] == '\0') {
            return 1U;
        }
    }
    return 0U;
}

static u64 x86_64_vfs_string_len(const char *value)
{
    u64 len = 0ULL;
    while (len < X86_64_VFS_PATH_MAX_BYTES && value[len] != '\0') {
        ++len;
    }
    return len;
}

static void x86_64_vfs_copy_path(char *dst, const char *src)
{
    u64 i = 0ULL;
    while (i + 1ULL < X86_64_VFS_PATH_MAX_BYTES && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static u32 x86_64_vfs_normalize_path(const char *path, char *normalized)
{
    if (path == (const char *)0 || normalized == (char *)0 || path[0] != '/') {
        return 0U;
    }

    u64 in = 0ULL;
    u64 out = 0ULL;
    normalized[out++] = '/';

    while (in < X86_64_VFS_PATH_MAX_BYTES && path[in] != '\0') {
        while (in < X86_64_VFS_PATH_MAX_BYTES && path[in] == '/') {
            ++in;
        }
        if (in >= X86_64_VFS_PATH_MAX_BYTES || path[in] == '\0') {
            break;
        }

        u64 segment_start = in;
        u64 segment_len = 0ULL;
        while (in < X86_64_VFS_PATH_MAX_BYTES && path[in] != '\0' && path[in] != '/') {
            ++in;
            ++segment_len;
        }
        if (in >= X86_64_VFS_PATH_MAX_BYTES) {
            return 0U;
        }
        if (segment_len == 1ULL && path[segment_start] == '.') {
            continue;
        }
        if (segment_len == 2ULL && path[segment_start] == '.' && path[segment_start + 1ULL] == '.') {
            if (out > 1ULL) {
                --out;
                while (out > 1ULL && normalized[out - 1ULL] != '/') {
                    --out;
                }
                normalized[out] = '\0';
            }
            continue;
        }
        if (out > 1ULL) {
            if (out + 1ULL >= X86_64_VFS_PATH_MAX_BYTES) {
                return 0U;
            }
            normalized[out++] = '/';
        }
        if (out + segment_len >= X86_64_VFS_PATH_MAX_BYTES) {
            return 0U;
        }
        for (u64 i = 0ULL; i < segment_len; ++i) {
            normalized[out++] = path[segment_start + i];
        }
        normalized[out] = '\0';
    }

    if (in >= X86_64_VFS_PATH_MAX_BYTES) {
        return 0U;
    }
    normalized[out] = '\0';
    return 1U;
}

static u32 x86_64_vfs_parent_path(const char *path, char *parent)
{
    u64 len = x86_64_vfs_string_len(path);
    if (len <= 1ULL) {
        return 0U;
    }

    u64 slash = len;
    while (slash > 1ULL && path[slash - 1ULL] != '/') {
        --slash;
    }
    if (slash == len) {
        return 0U;
    }
    if (slash == 1ULL) {
        parent[0] = '/';
        parent[1] = '\0';
        return 1U;
    }

    for (u64 i = 0ULL; i < slash - 1ULL; ++i) {
        parent[i] = path[i];
    }
    parent[slash - 1ULL] = '\0';
    return 1U;
}

static u32 x86_64_vfs_is_tmp_path(const char *path)
{
    return (path[0] == '/' && path[1] == 't' && path[2] == 'm' &&
            path[3] == 'p' && path[4] == '/') ? 1U : 0U;
}

static u32 x86_64_vfs_static_find_node(const char *path)
{
    for (u32 i = 0U; i < X86_64_VFS_STATIC_NODE_COUNT; ++i) {
        if (x86_64_vfs_path_equals(path, x86_64_vfs_nodes[i].path) != 0U) {
            return i;
        }
    }
    return X86_64_VFS_NO_NODE;
}

static u32 x86_64_vfs_ram_find_node(const char *path)
{
    for (u32 i = 0U; i < X86_64_VFS_RAM_NODE_COUNT; ++i) {
        if (x86_64_vfs_ram_nodes[i].used != 0U &&
            x86_64_vfs_path_equals(path, x86_64_vfs_ram_nodes[i].path) != 0U) {
            return X86_64_VFS_RAM_NODE_BASE + i;
        }
    }
    return X86_64_VFS_NO_NODE;
}

static u32 x86_64_vfs_find_normalized_node(const char *normalized)
{
    u32 static_index = x86_64_vfs_static_find_node(normalized);
    if (static_index != X86_64_VFS_NO_NODE) {
        return static_index;
    }
    return x86_64_vfs_ram_find_node(normalized);
}

static u32 x86_64_vfs_find_node(const char *path)
{
    char normalized[X86_64_VFS_PATH_MAX_BYTES];
    if (x86_64_vfs_normalize_path(path, normalized) == 0U) {
        return X86_64_VFS_NO_NODE;
    }
    return x86_64_vfs_find_normalized_node(normalized);
}

static u32 x86_64_vfs_node_type(u32 index)
{
    if (index < X86_64_VFS_STATIC_NODE_COUNT) {
        return x86_64_vfs_nodes[index].type;
    }
    if (x86_64_vfs_is_ram_index(index) != 0U) {
        return x86_64_vfs_ram_nodes[index - X86_64_VFS_RAM_NODE_BASE].type;
    }
    return 0U;
}

static const char *x86_64_vfs_node_path(u32 index)
{
    if (index < X86_64_VFS_STATIC_NODE_COUNT) {
        return x86_64_vfs_nodes[index].path;
    }
    if (x86_64_vfs_is_ram_index(index) != 0U) {
        return x86_64_vfs_ram_nodes[index - X86_64_VFS_RAM_NODE_BASE].path;
    }
    return (const char *)0;
}

static u32 x86_64_vfs_is_session_node(u32 index)
{
    const char *path = x86_64_vfs_node_path(index);
    return (path != (const char *)0 && x86_64_vfs_path_equals(path, "/tmp/session.txt") != 0U) ? 1U : 0U;
}

static u32 x86_64_vfs_directory_empty(const char *directory)
{
    u64 dir_len = x86_64_vfs_string_len(directory);
    for (u32 i = 0U; i < X86_64_VFS_RAM_NODE_COUNT; ++i) {
        if (x86_64_vfs_ram_nodes[i].used == 0U) {
            continue;
        }
        const char *path = x86_64_vfs_ram_nodes[i].path;
        if (x86_64_vfs_string_len(path) <= dir_len + 1ULL) {
            continue;
        }
        u32 prefix = 1U;
        for (u64 j = 0ULL; j < dir_len; ++j) {
            if (path[j] != directory[j]) {
                prefix = 0U;
                break;
            }
        }
        if (prefix != 0U && path[dir_len] == '/') {
            return 0U;
        }
    }
    return 1U;
}

static u32 x86_64_vfs_parent_ready(const char *normalized)
{
    char parent[X86_64_VFS_PATH_MAX_BYTES];
    if (x86_64_vfs_parent_path(normalized, parent) == 0U) {
        return 0U;
    }
    u32 parent_index = x86_64_vfs_find_normalized_node(parent);
    return (parent_index != X86_64_VFS_NO_NODE &&
            x86_64_vfs_node_type(parent_index) == X86_64_VFS_NODE_DIRECTORY) ? 1U : 0U;
}

static u64 x86_64_vfs_create_ram_node(const char *normalized, u32 type, u32 *index_out)
{
    if (x86_64_vfs_is_tmp_path(normalized) == 0U ||
        x86_64_vfs_find_normalized_node(normalized) != X86_64_VFS_NO_NODE ||
        x86_64_vfs_parent_ready(normalized) == 0U) {
        return X86_64_VFS_RET_EINVAL;
    }

    for (u32 i = 0U; i < X86_64_VFS_RAM_NODE_COUNT; ++i) {
        if (x86_64_vfs_ram_nodes[i].used == 0U) {
            x86_64_vfs_ram_nodes[i].used = 1U;
            x86_64_vfs_ram_nodes[i].type = type;
            x86_64_vfs_copy_path(x86_64_vfs_ram_nodes[i].path, normalized);
            x86_64_vfs_ram_nodes[i].size = 0ULL;
            if (index_out != (u32 *)0) {
                *index_out = X86_64_VFS_RAM_NODE_BASE + i;
            }
            return X86_64_VFS_RET_OK;
        }
    }

    return X86_64_VFS_RET_ENOSPC;
}

static u64 x86_64_vfs_append_listing(char *buffer, u64 offset, const char *value)
{
    for (u64 i = 0ULL; value[i] != '\0' && offset + 1ULL < X86_64_VFS_DIR_LIST_BYTES; ++i) {
        buffer[offset++] = value[i];
    }
    return offset;
}

static u32 x86_64_vfs_is_immediate_child(const char *parent, const char *child)
{
    u64 parent_len = x86_64_vfs_string_len(parent);
    if (x86_64_vfs_string_len(child) <= parent_len + 1ULL) {
        return 0U;
    }
    for (u64 i = 0ULL; i < parent_len; ++i) {
        if (parent[i] != child[i]) {
            return 0U;
        }
    }
    if (child[parent_len] != '/') {
        return 0U;
    }
    for (u64 i = parent_len + 1ULL; child[i] != '\0'; ++i) {
        if (child[i] == '/') {
            return 0U;
        }
    }
    return 1U;
}

static const char *x86_64_vfs_basename(const char *path)
{
    u64 last = 0ULL;
    for (u64 i = 0ULL; i < X86_64_VFS_PATH_MAX_BYTES && path[i] != '\0'; ++i) {
        if (path[i] == '/') {
            last = i + 1ULL;
        }
    }
    return path + last;
}

static u64 x86_64_vfs_build_directory_listing(const char *directory)
{
    u64 offset = 0ULL;
    for (u64 i = 0ULL; i < X86_64_VFS_DIR_LIST_BYTES; ++i) {
        x86_64_vfs_dir_listing[i] = '\0';
    }

    if (x86_64_vfs_path_equals(directory, "/tmp") != 0U) {
        offset = x86_64_vfs_append_listing(x86_64_vfs_dir_listing, offset, "session.txt\n");
    }

    for (u32 i = 0U; i < X86_64_VFS_RAM_NODE_COUNT; ++i) {
        if (x86_64_vfs_ram_nodes[i].used == 0U) {
            continue;
        }
        if (x86_64_vfs_is_immediate_child(directory, x86_64_vfs_ram_nodes[i].path) == 0U) {
            continue;
        }
        offset = x86_64_vfs_append_listing(x86_64_vfs_dir_listing, offset,
                                           x86_64_vfs_basename(x86_64_vfs_ram_nodes[i].path));
        offset = x86_64_vfs_append_listing(x86_64_vfs_dir_listing, offset, "\n");
    }

    return offset;
}

static u64 x86_64_vfs_executable_size(const u8 *data)
{
    if (data == x86_64_user_eventd_image_start) {
        return (u64)(x86_64_user_eventd_image_end - x86_64_user_eventd_image_start);
    }
    if (data == x86_64_user_datatest_image_start) {
        return (u64)(x86_64_user_datatest_image_end - x86_64_user_datatest_image_start);
    }
    if (data == x86_64_user_file_manager_image_start) {
        return (u64)(x86_64_user_file_manager_image_end - x86_64_user_file_manager_image_start);
    }
    if (data == x86_64_user_hello_image_start) {
        return (u64)(x86_64_user_hello_image_end - x86_64_user_hello_image_start);
    }
    if (data == x86_64_user_inputd_image_start) {
        return (u64)(x86_64_user_inputd_image_end - x86_64_user_inputd_image_start);
    }
    if (data == x86_64_user_sessiond_image_start) {
        return (u64)(x86_64_user_sessiond_image_end - x86_64_user_sessiond_image_start);
    }
    if (data == x86_64_user_settings_image_start) {
        return (u64)(x86_64_user_settings_image_end - x86_64_user_settings_image_start);
    }
    if (data == x86_64_user_storaged_image_start) {
        return (u64)(x86_64_user_storaged_image_end - x86_64_user_storaged_image_start);
    }
    if (data == x86_64_user_sysinfo_image_start) {
        return (u64)(x86_64_user_sysinfo_image_end - x86_64_user_sysinfo_image_start);
    }
    if (data == x86_64_user_system_monitor_image_start) {
        return (u64)(x86_64_user_system_monitor_image_end - x86_64_user_system_monitor_image_start);
    }
    if (data == x86_64_user_terminal_image_start) {
        return (u64)(x86_64_user_terminal_image_end - x86_64_user_terminal_image_start);
    }
    if (data == x86_64_user_text_editor_image_start) {
        return (u64)(x86_64_user_text_editor_image_end - x86_64_user_text_editor_image_start);
    }
    if (data == x86_64_user_timed_image_start) {
        return (u64)(x86_64_user_timed_image_end - x86_64_user_timed_image_start);
    }
    if (data == x86_64_user_windowd_image_start) {
        return (u64)(x86_64_user_windowd_image_end - x86_64_user_windowd_image_start);
    }
    if (data == x86_64_user_windowd_service_image_start) {
        return (u64)(x86_64_user_windowd_service_image_end - x86_64_user_windowd_service_image_start);
    }
    return 0ULL;
}

static u64 x86_64_vfs_node_size_by_index(u32 index)
{
    if (index < X86_64_VFS_STATIC_NODE_COUNT) {
        if (x86_64_vfs_is_session_node(index) != 0U) {
            return x86_64_vfs_session_storage_size;
        }
        if (x86_64_vfs_nodes[index].type == X86_64_VFS_NODE_DIRECTORY &&
            x86_64_vfs_nodes[index].data == (const u8 *)0) {
            return x86_64_vfs_build_directory_listing(x86_64_vfs_nodes[index].path);
        }
        if (x86_64_vfs_nodes[index].type == X86_64_VFS_NODE_EXECUTABLE) {
            return x86_64_vfs_executable_size(x86_64_vfs_nodes[index].data);
        }
        return x86_64_vfs_nodes[index].size;
    }

    if (x86_64_vfs_is_ram_index(index) != 0U) {
        struct x86_64_vfs_ram_node *node = &x86_64_vfs_ram_nodes[index - X86_64_VFS_RAM_NODE_BASE];
        if (node->type == X86_64_VFS_NODE_DIRECTORY) {
            return x86_64_vfs_build_directory_listing(node->path);
        }
        return node->size;
    }

    return 0ULL;
}

static const u8 *x86_64_vfs_node_data_by_index(u32 index)
{
    if (index < X86_64_VFS_STATIC_NODE_COUNT) {
        if (x86_64_vfs_is_session_node(index) != 0U) {
            return x86_64_vfs_session_storage;
        }
        if (x86_64_vfs_nodes[index].type == X86_64_VFS_NODE_DIRECTORY &&
            x86_64_vfs_nodes[index].data == (const u8 *)0) {
            (void)x86_64_vfs_build_directory_listing(x86_64_vfs_nodes[index].path);
            return (const u8 *)x86_64_vfs_dir_listing;
        }
        return x86_64_vfs_nodes[index].data;
    }

    if (x86_64_vfs_is_ram_index(index) != 0U) {
        struct x86_64_vfs_ram_node *node = &x86_64_vfs_ram_nodes[index - X86_64_VFS_RAM_NODE_BASE];
        if (node->type == X86_64_VFS_NODE_DIRECTORY) {
            (void)x86_64_vfs_build_directory_listing(node->path);
            return (const u8 *)x86_64_vfs_dir_listing;
        }
        return node->data;
    }

    return (const u8 *)0;
}

static u32 x86_64_vfs_fd_slot(u64 fd)
{
    if (fd < X86_64_VFS_FD_BASE) {
        return X86_64_VFS_MAX_OPEN_FILES;
    }
    u64 slot = fd - X86_64_VFS_FD_BASE;
    return (slot < (u64)X86_64_VFS_MAX_OPEN_FILES) ? (u32)slot : X86_64_VFS_MAX_OPEN_FILES;
}

void x86_64_vfs_init(struct x86_64_vfs_state *state)
{
    state->initialized = 1U;
    for (u32 i = 0U; i < X86_64_VFS_MAX_OPEN_FILES; ++i) {
        state->fd_used[i] = 0U;
        state->fd_flags[i] = X86_64_VFS_OPEN_READ;
        state->fd_node_index[i] = X86_64_VFS_NO_NODE;
        state->fd_offset[i] = 0ULL;
    }
}

u64 x86_64_vfs_open(struct x86_64_vfs_state *state, const char *path, u64 flags)
{
    if (state == (struct x86_64_vfs_state *)0 || path == (const char *)0) {
        return X86_64_VFS_RET_EINVAL;
    }

    char normalized[X86_64_VFS_PATH_MAX_BYTES];
    if (x86_64_vfs_normalize_path(path, normalized) == 0U) {
        return X86_64_VFS_RET_EINVAL;
    }

    u64 supported_flags = X86_64_VFS_OPEN_WRITE | X86_64_VFS_OPEN_CREATE | X86_64_VFS_OPEN_TRUNCATE;
    if ((flags & ~supported_flags) != 0ULL) {
        return X86_64_VFS_RET_EINVAL;
    }

    u32 node_index = x86_64_vfs_find_normalized_node(normalized);
    if (node_index == X86_64_VFS_NO_NODE) {
        if ((flags & X86_64_VFS_OPEN_CREATE) == 0ULL) {
            return X86_64_VFS_RET_ENOENT;
        }
        u64 create_result = x86_64_vfs_create_ram_node(normalized, X86_64_VFS_NODE_FILE, &node_index);
        if (create_result != X86_64_VFS_RET_OK) {
            return create_result;
        }
    }

    u32 node_type = x86_64_vfs_node_type(node_index);
    if ((flags & X86_64_VFS_OPEN_WRITE) != 0ULL) {
        if (node_type != X86_64_VFS_NODE_FILE ||
            (x86_64_vfs_is_ram_index(node_index) == 0U && x86_64_vfs_is_session_node(node_index) == 0U)) {
            return X86_64_VFS_RET_EINVAL;
        }
    }

    if ((flags & X86_64_VFS_OPEN_TRUNCATE) != 0ULL) {
        if (x86_64_vfs_is_session_node(node_index) != 0U) {
            x86_64_vfs_session_storage_size = 0ULL;
        } else if (x86_64_vfs_is_ram_index(node_index) != 0U) {
            x86_64_vfs_ram_nodes[node_index - X86_64_VFS_RAM_NODE_BASE].size = 0ULL;
        } else {
            return X86_64_VFS_RET_EINVAL;
        }
    }

    for (u32 slot = 0U; slot < X86_64_VFS_MAX_OPEN_FILES; ++slot) {
        if (state->fd_used[slot] == 0U) {
            state->fd_used[slot] = 1U;
            state->fd_flags[slot] = flags;
            state->fd_node_index[slot] = node_index;
            state->fd_offset[slot] = 0ULL;
            return X86_64_VFS_FD_BASE + (u64)slot;
        }
    }

    return X86_64_VFS_RET_EINVAL;
}

u64 x86_64_vfs_read(struct x86_64_vfs_state *state, u64 fd, char *buffer, u64 size)
{
    if (state == (struct x86_64_vfs_state *)0 || buffer == (char *)0) {
        return X86_64_VFS_RET_EINVAL;
    }
    u32 slot = x86_64_vfs_fd_slot(fd);
    if (slot >= X86_64_VFS_MAX_OPEN_FILES || state->fd_used[slot] == 0U) {
        return X86_64_VFS_RET_EBADF;
    }

    u32 node_index = state->fd_node_index[slot];
    const u8 *node_data = x86_64_vfs_node_data_by_index(node_index);
    u64 node_size = x86_64_vfs_node_size_by_index(node_index);
    u64 offset = state->fd_offset[slot];
    if (node_data == (const u8 *)0 || offset >= node_size || size == 0ULL) {
        return 0ULL;
    }

    u64 remaining = node_size - offset;
    u64 bytes = (size < remaining) ? size : remaining;
    for (u64 i = 0ULL; i < bytes; ++i) {
        buffer[i] = (char)node_data[offset + i];
    }
    state->fd_offset[slot] = offset + bytes;
    return bytes;
}

u64 x86_64_vfs_write(struct x86_64_vfs_state *state, u64 fd, const char *buffer, u64 size)
{
    if (state == (struct x86_64_vfs_state *)0 || buffer == (const char *)0) {
        return X86_64_VFS_RET_EINVAL;
    }
    u32 slot = x86_64_vfs_fd_slot(fd);
    if (slot >= X86_64_VFS_MAX_OPEN_FILES || state->fd_used[slot] == 0U ||
        (state->fd_flags[slot] & X86_64_VFS_OPEN_WRITE) == 0ULL) {
        return X86_64_VFS_RET_EBADF;
    }

    u32 node_index = state->fd_node_index[slot];
    if (x86_64_vfs_node_type(node_index) != X86_64_VFS_NODE_FILE) {
        return X86_64_VFS_RET_EBADF;
    }

    u8 *target = (u8 *)0;
    u64 *target_size = (u64 *)0;
    if (x86_64_vfs_is_session_node(node_index) != 0U) {
        target = x86_64_vfs_session_storage;
        target_size = &x86_64_vfs_session_storage_size;
    } else if (x86_64_vfs_is_ram_index(node_index) != 0U) {
        struct x86_64_vfs_ram_node *node = &x86_64_vfs_ram_nodes[node_index - X86_64_VFS_RAM_NODE_BASE];
        target = node->data;
        target_size = &node->size;
    } else {
        return X86_64_VFS_RET_EBADF;
    }

    u64 offset = state->fd_offset[slot];
    if (offset >= X86_64_VFS_RAM_FILE_BYTES || size == 0ULL) {
        return 0ULL;
    }
    u64 remaining = X86_64_VFS_RAM_FILE_BYTES - offset;
    u64 bytes = (size < remaining) ? size : remaining;
    for (u64 i = 0ULL; i < bytes; ++i) {
        target[offset + i] = (u8)buffer[i];
    }
    state->fd_offset[slot] = offset + bytes;
    if (*target_size < state->fd_offset[slot]) {
        *target_size = state->fd_offset[slot];
    }
    return bytes;
}

u64 x86_64_vfs_close(struct x86_64_vfs_state *state, u64 fd)
{
    if (state == (struct x86_64_vfs_state *)0) {
        return X86_64_VFS_RET_EINVAL;
    }
    u32 slot = x86_64_vfs_fd_slot(fd);
    if (slot >= X86_64_VFS_MAX_OPEN_FILES || state->fd_used[slot] == 0U) {
        return X86_64_VFS_RET_EBADF;
    }
    state->fd_used[slot] = 0U;
    state->fd_flags[slot] = X86_64_VFS_OPEN_READ;
    state->fd_node_index[slot] = X86_64_VFS_NO_NODE;
    state->fd_offset[slot] = 0ULL;
    return X86_64_VFS_RET_OK;
}

u64 x86_64_vfs_stat(const struct x86_64_vfs_state *state, const char *path, u64 *size_out)
{
    if (state == (const struct x86_64_vfs_state *)0 || path == (const char *)0 || size_out == (u64 *)0) {
        return X86_64_VFS_RET_EINVAL;
    }
    u32 node_index = x86_64_vfs_find_node(path);
    if (node_index == X86_64_VFS_NO_NODE) {
        return X86_64_VFS_RET_ENOENT;
    }
    *size_out = x86_64_vfs_node_size_by_index(node_index);
    return X86_64_VFS_RET_OK;
}

u64 x86_64_vfs_type(const struct x86_64_vfs_state *state, const char *path, u64 *type_out)
{
    if (state == (const struct x86_64_vfs_state *)0 || path == (const char *)0 || type_out == (u64 *)0) {
        return X86_64_VFS_RET_EINVAL;
    }
    u32 node_index = x86_64_vfs_find_node(path);
    if (node_index == X86_64_VFS_NO_NODE) {
        return X86_64_VFS_RET_ENOENT;
    }
    *type_out = (u64)x86_64_vfs_node_type(node_index);
    return X86_64_VFS_RET_OK;
}

u64 x86_64_vfs_mkdir(struct x86_64_vfs_state *state, const char *path)
{
    if (state == (struct x86_64_vfs_state *)0 || path == (const char *)0) {
        return X86_64_VFS_RET_EINVAL;
    }
    char normalized[X86_64_VFS_PATH_MAX_BYTES];
    if (x86_64_vfs_normalize_path(path, normalized) == 0U) {
        return X86_64_VFS_RET_EINVAL;
    }
    return x86_64_vfs_create_ram_node(normalized, X86_64_VFS_NODE_DIRECTORY, (u32 *)0);
}

u64 x86_64_vfs_unlink(struct x86_64_vfs_state *state, const char *path)
{
    if (state == (struct x86_64_vfs_state *)0 || path == (const char *)0) {
        return X86_64_VFS_RET_EINVAL;
    }
    char normalized[X86_64_VFS_PATH_MAX_BYTES];
    if (x86_64_vfs_normalize_path(path, normalized) == 0U) {
        return X86_64_VFS_RET_EINVAL;
    }
    u32 node_index = x86_64_vfs_find_normalized_node(normalized);
    if (node_index == X86_64_VFS_NO_NODE) {
        return X86_64_VFS_RET_ENOENT;
    }
    if (x86_64_vfs_is_ram_index(node_index) == 0U) {
        return X86_64_VFS_RET_EINVAL;
    }
    struct x86_64_vfs_ram_node *node = &x86_64_vfs_ram_nodes[node_index - X86_64_VFS_RAM_NODE_BASE];
    if (node->type == X86_64_VFS_NODE_DIRECTORY && x86_64_vfs_directory_empty(node->path) == 0U) {
        return X86_64_VFS_RET_ENOTEMPTY;
    }
    node->used = 0U;
    node->type = 0U;
    node->size = 0ULL;
    node->path[0] = '\0';
    return X86_64_VFS_RET_OK;
}

u64 x86_64_vfs_resolve(const struct x86_64_vfs_state *state, const char *path,
                       const u8 **data_out, u64 *size_out, u64 *type_out)
{
    if (state == (const struct x86_64_vfs_state *)0 || state->initialized == 0U ||
        path == (const char *)0 || data_out == (const u8 **)0 ||
        size_out == (u64 *)0 || type_out == (u64 *)0) {
        return X86_64_VFS_RET_EINVAL;
    }
    u32 node_index = x86_64_vfs_find_node(path);
    if (node_index == X86_64_VFS_NO_NODE) {
        return X86_64_VFS_RET_ENOENT;
    }
    *data_out = x86_64_vfs_node_data_by_index(node_index);
    *size_out = x86_64_vfs_node_size_by_index(node_index);
    *type_out = (u64)x86_64_vfs_node_type(node_index);
    return X86_64_VFS_RET_OK;
}

u32 x86_64_vfs_file_count(void)
{
    u32 count = X86_64_VFS_STATIC_NODE_COUNT;
    for (u32 i = 0U; i < X86_64_VFS_RAM_NODE_COUNT; ++i) {
        if (x86_64_vfs_ram_nodes[i].used != 0U) {
            count += 1U;
        }
    }
    return count;
}

u32 x86_64_vfs_open_count(const struct x86_64_vfs_state *state)
{
    u32 count = 0U;
    for (u32 i = 0U; i < X86_64_VFS_MAX_OPEN_FILES; ++i) {
        if (state->fd_used[i] != 0U) {
            count += 1U;
        }
    }
    return count;
}

u32 x86_64_vfs_ready(const struct x86_64_vfs_state *state)
{
    if (state == (const struct x86_64_vfs_state *)0 || state->initialized == 0U) {
        return 0U;
    }

    struct x86_64_vfs_state probe;
    char buffer[16];
    u64 size = 0ULL;
    u64 type = 0ULL;
    const char config_value[] = "theme=system\n";

    x86_64_vfs_init(&probe);
    u64 root_fd = x86_64_vfs_open(&probe, "/", 0ULL);
    if (root_fd != X86_64_VFS_FD_BASE) {
        return 0U;
    }
    if (x86_64_vfs_read(&probe, root_fd, buffer, 4ULL) != 4ULL ||
        buffer[0] != 'b' || buffer[1] != 'i' || buffer[2] != 'n' || buffer[3] != '\n') {
        return 0U;
    }
    if (x86_64_vfs_close(&probe, root_fd) != X86_64_VFS_RET_OK) {
        return 0U;
    }

    if (x86_64_vfs_type(&probe, "/etc/desktop.conf", &type) != X86_64_VFS_RET_OK ||
        type != X86_64_VFS_NODE_FILE) {
        return 0U;
    }
    if (x86_64_vfs_type(&probe, "/usr/share", &type) != X86_64_VFS_RET_OK ||
        type != X86_64_VFS_NODE_DIRECTORY) {
        return 0U;
    }
    if (x86_64_vfs_type(&probe, "/bin/file-manager", &type) != X86_64_VFS_RET_OK ||
        type != X86_64_VFS_NODE_EXECUTABLE) {
        return 0U;
    }
    if (x86_64_vfs_stat(&probe, "/bin/terminal", &size) != X86_64_VFS_RET_OK || size <= 64ULL) {
        return 0U;
    }
    if (x86_64_vfs_stat(&probe, "/tmp/session.txt", &size) != X86_64_VFS_RET_OK ||
        size > X86_64_VFS_RAM_FILE_BYTES) {
        return 0U;
    }

    if (x86_64_vfs_mkdir(&probe, "/tmp/config") != X86_64_VFS_RET_OK) {
        return 0U;
    }
    if (x86_64_vfs_type(&probe, "/tmp/config", &type) != X86_64_VFS_RET_OK ||
        type != X86_64_VFS_NODE_DIRECTORY) {
        return 0U;
    }
    u64 fd = x86_64_vfs_open(&probe, "/tmp/config/desktop.conf",
                             X86_64_VFS_OPEN_WRITE | X86_64_VFS_OPEN_CREATE | X86_64_VFS_OPEN_TRUNCATE);
    if (fd < X86_64_VFS_FD_BASE) {
        return 0U;
    }
    if (x86_64_vfs_write(&probe, fd, config_value, sizeof(config_value) - 1ULL) != sizeof(config_value) - 1ULL) {
        return 0U;
    }
    if (x86_64_vfs_close(&probe, fd) != X86_64_VFS_RET_OK) {
        return 0U;
    }
    fd = x86_64_vfs_open(&probe, "/tmp/config/desktop.conf", X86_64_VFS_OPEN_READ);
    if (fd < X86_64_VFS_FD_BASE) {
        return 0U;
    }
    if (x86_64_vfs_read(&probe, fd, buffer, 6ULL) != 6ULL ||
        buffer[0] != 't' || buffer[1] != 'h' || buffer[2] != 'e' ||
        buffer[3] != 'm' || buffer[4] != 'e' || buffer[5] != '=') {
        return 0U;
    }
    if (x86_64_vfs_close(&probe, fd) != X86_64_VFS_RET_OK) {
        return 0U;
    }
    if (x86_64_vfs_unlink(&probe, "/tmp/config/desktop.conf") != X86_64_VFS_RET_OK) {
        return 0U;
    }
    if (x86_64_vfs_stat(&probe, "/tmp/config/desktop.conf", &size) != X86_64_VFS_RET_ENOENT) {
        return 0U;
    }
    if (x86_64_vfs_unlink(&probe, "/tmp/config") != X86_64_VFS_RET_OK) {
        return 0U;
    }

    return 1U;
}

u32 x86_64_vfs_session_storage_ready(void)
{
    struct x86_64_vfs_state probe;
    char buffer[4];
    const char value[] = "ram";
    u8 saved[X86_64_VFS_RAM_FILE_BYTES];
    u64 saved_size = x86_64_vfs_session_storage_size;
    u32 ok = 0U;

    for (u64 i = 0ULL; i < X86_64_VFS_RAM_FILE_BYTES; ++i) {
        saved[i] = x86_64_vfs_session_storage[i];
    }

    x86_64_vfs_init(&probe);
    u64 fd = x86_64_vfs_open(&probe, "/tmp/session.txt",
                             X86_64_VFS_OPEN_WRITE | X86_64_VFS_OPEN_CREATE | X86_64_VFS_OPEN_TRUNCATE);
    if (fd < X86_64_VFS_FD_BASE) {
        goto restore;
    }
    if (x86_64_vfs_write(&probe, fd, value, 3ULL) != 3ULL) {
        goto restore;
    }
    if (x86_64_vfs_close(&probe, fd) != X86_64_VFS_RET_OK) {
        goto restore;
    }
    fd = x86_64_vfs_open(&probe, "/tmp/session.txt", X86_64_VFS_OPEN_READ);
    if (fd < X86_64_VFS_FD_BASE) {
        goto restore;
    }
    if (x86_64_vfs_read(&probe, fd, buffer, 3ULL) != 3ULL ||
        buffer[0] != 'r' || buffer[1] != 'a' || buffer[2] != 'm') {
        goto restore;
    }
    ok = (x86_64_vfs_close(&probe, fd) == X86_64_VFS_RET_OK) ? 1U : 0U;

restore:
    for (u64 i = 0ULL; i < X86_64_VFS_RAM_FILE_BYTES; ++i) {
        x86_64_vfs_session_storage[i] = saved[i];
    }
    x86_64_vfs_session_storage_size = saved_size;
    return ok;
}

u32 x86_64_vfs_ram_storage_ready(void)
{
    struct x86_64_vfs_state probe;
    x86_64_vfs_init(&probe);
    return x86_64_vfs_ready(&probe);
}

u32 x86_64_vfs_persistent_storage_ready(void)
{
    return 0U;
}
