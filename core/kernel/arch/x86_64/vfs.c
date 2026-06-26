#include "vfs.h"

extern const u8 x86_64_user_hello_image_start[];
extern const u8 x86_64_user_hello_image_end[];
extern const u8 x86_64_user_sysinfo_image_start[];
extern const u8 x86_64_user_sysinfo_image_end[];

struct x86_64_vfs_node {
    const char *path;
    const u8 *data;
    u64 size;
    u32 type;
};

static const char x86_64_vfs_dir_root[] =
    "bin\n"
    "etc\n"
    "proc\n"
    "sbin\n"
    "usr\n";

static const char x86_64_vfs_dir_bin[] =
    "hello\n"
    "sysinfo\n";
static const char x86_64_vfs_dir_sbin[] = "";

static const char x86_64_vfs_dir_etc[] =
    "motd\n"
    "os-release\n";

static const char x86_64_vfs_dir_proc[] =
    "version\n";

static const char x86_64_vfs_dir_usr[] =
    "share\n";

static const char x86_64_vfs_dir_usr_share[] =
    "files.txt\n"
    "help.txt\n";

static const char x86_64_vfs_file_os_release[] =
    "NAME=Liam_OS\n"
    "ARCH=x86_64\n"
    "BUILD=dev\n";

static const char x86_64_vfs_file_motd[] =
    "Liam_OS x86_64 read-only VFS mounted.\n";

static const char x86_64_vfs_file_version[] =
    "Liam_OS Core x86_64 dev\n";

static const char x86_64_vfs_file_help[] =
    "Available commands:\n"
    "help, about, version, pid, ps, wait, echo, ls, cat, stat, exec, clear, exit\n"
    "Executable files:\n"
    "/bin/hello\n"
    "/bin/sysinfo\n";

static const char x86_64_vfs_file_files[] =
    "/\n"
    "/bin\n"
    "/bin/hello\n"
    "/bin/sysinfo\n"
    "/etc\n"
    "/etc/motd\n"
    "/etc/os-release\n"
    "/proc\n"
    "/proc/version\n"
    "/sbin\n"
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
    {"/usr", (const u8 *)x86_64_vfs_dir_usr, sizeof(x86_64_vfs_dir_usr) - 1ULL, X86_64_VFS_NODE_DIRECTORY},
    {"/usr/share", (const u8 *)x86_64_vfs_dir_usr_share, sizeof(x86_64_vfs_dir_usr_share) - 1ULL, X86_64_VFS_NODE_DIRECTORY},
    {"/bin/hello", x86_64_user_hello_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/bin/sysinfo", x86_64_user_sysinfo_image_start, 0ULL, X86_64_VFS_NODE_EXECUTABLE},
    {"/etc/motd", (const u8 *)x86_64_vfs_file_motd, sizeof(x86_64_vfs_file_motd) - 1ULL, X86_64_VFS_NODE_FILE},
    {"/etc/os-release", (const u8 *)x86_64_vfs_file_os_release, sizeof(x86_64_vfs_file_os_release) - 1ULL, X86_64_VFS_NODE_FILE},
    {"/proc/version", (const u8 *)x86_64_vfs_file_version, sizeof(x86_64_vfs_file_version) - 1ULL, X86_64_VFS_NODE_FILE},
    {"/usr/share/files.txt", (const u8 *)x86_64_vfs_file_files, sizeof(x86_64_vfs_file_files) - 1ULL, X86_64_VFS_NODE_FILE},
    {"/usr/share/help.txt", (const u8 *)x86_64_vfs_file_help, sizeof(x86_64_vfs_file_help) - 1ULL, X86_64_VFS_NODE_FILE},
};

#define X86_64_VFS_NODE_COUNT ((u32)(sizeof(x86_64_vfs_nodes) / sizeof(x86_64_vfs_nodes[0])))

static u64 x86_64_vfs_node_size(const struct x86_64_vfs_node *node)
{
    if (node->data == x86_64_user_hello_image_start) {
        return (u64)(x86_64_user_hello_image_end - x86_64_user_hello_image_start);
    }

    if (node->data == x86_64_user_sysinfo_image_start) {
        return (u64)(x86_64_user_sysinfo_image_end - x86_64_user_sysinfo_image_start);
    }

    return node->size;
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

static u32 x86_64_vfs_path_equals(const char *path, const char *kernel_path)
{
    for (u64 i = 0ULL; i < X86_64_VFS_PATH_MAX_BYTES; ++i) {
        char path_char = path[i];
        char kernel_char = kernel_path[i];
        if (path_char != kernel_char) {
            return 0U;
        }
        if (path_char == '\0') {
            return 1U;
        }
    }

    return 0U;
}

static u32 x86_64_vfs_find_node(const char *path)
{
    char normalized[X86_64_VFS_PATH_MAX_BYTES];
    if (x86_64_vfs_normalize_path(path, normalized) == 0U) {
        return X86_64_VFS_NO_NODE;
    }

    for (u32 i = 0U; i < X86_64_VFS_NODE_COUNT; ++i) {
        if (x86_64_vfs_path_equals(normalized, x86_64_vfs_nodes[i].path) != 0U) {
            return i;
        }
    }

    return X86_64_VFS_NO_NODE;
}

static u32 x86_64_vfs_fd_slot(u64 fd)
{
    if (fd < X86_64_VFS_FD_BASE) {
        return X86_64_VFS_MAX_OPEN_FILES;
    }

    u64 slot = fd - X86_64_VFS_FD_BASE;
    if (slot >= (u64)X86_64_VFS_MAX_OPEN_FILES) {
        return X86_64_VFS_MAX_OPEN_FILES;
    }

    return (u32)slot;
}

void x86_64_vfs_init(struct x86_64_vfs_state *state)
{
    state->initialized = 1U;
    for (u32 i = 0U; i < X86_64_VFS_MAX_OPEN_FILES; ++i) {
        state->fd_used[i] = 0U;
        state->fd_node_index[i] = X86_64_VFS_NO_NODE;
        state->fd_offset[i] = 0ULL;
    }
}

u32 x86_64_vfs_ready(const struct x86_64_vfs_state *state)
{
    if (state == (const struct x86_64_vfs_state *)0 ||
        state->initialized == 0U ||
        X86_64_VFS_NODE_COUNT == 0U) {
        return 0U;
    }

    struct x86_64_vfs_state probe;
    char buffer[6];
    u64 size = 0ULL;
    u64 type = 0ULL;

    x86_64_vfs_init(&probe);
    u64 root_fd = x86_64_vfs_open(&probe, "/", 0ULL);
    if (root_fd != X86_64_VFS_FD_BASE) {
        return 0U;
    }

    u64 root_bytes = x86_64_vfs_read(&probe, root_fd, buffer, 4ULL);
    if (root_bytes != 4ULL ||
        buffer[0] != 'b' ||
        buffer[1] != 'i' ||
        buffer[2] != 'n' ||
        buffer[3] != '\n') {
        return 0U;
    }

    if (x86_64_vfs_close(&probe, root_fd) != X86_64_VFS_RET_OK) {
        return 0U;
    }

    u64 bin_fd = x86_64_vfs_open(&probe, "///usr/../bin/./", 0ULL);
    if (bin_fd != X86_64_VFS_FD_BASE) {
        return 0U;
    }

    u64 bin_bytes = x86_64_vfs_read(&probe, bin_fd, buffer, sizeof(buffer));
    if (bin_bytes != sizeof(buffer) ||
        buffer[0] != 'h' ||
        buffer[1] != 'e' ||
        buffer[2] != 'l' ||
        buffer[3] != 'l' ||
        buffer[4] != 'o' ||
        buffer[5] != '\n') {
        return 0U;
    }

    if (x86_64_vfs_close(&probe, bin_fd) != X86_64_VFS_RET_OK) {
        return 0U;
    }

    u64 fd = x86_64_vfs_open(&probe, "/etc/./os-release", 0ULL);
    if (fd != X86_64_VFS_FD_BASE) {
        return 0U;
    }

    u64 bytes = x86_64_vfs_read(&probe, fd, buffer, 5ULL);
    if (bytes != 5ULL ||
        buffer[0] != 'N' ||
        buffer[1] != 'A' ||
        buffer[2] != 'M' ||
        buffer[3] != 'E' ||
        buffer[4] != '=') {
        return 0U;
    }

    if (x86_64_vfs_stat(&probe, "/etc/../etc/os-release", &size) != X86_64_VFS_RET_OK ||
        size != (sizeof(x86_64_vfs_file_os_release) - 1ULL)) {
        return 0U;
    }

    if (x86_64_vfs_type(&probe, "/etc/os-release", &type) != X86_64_VFS_RET_OK ||
        type != X86_64_VFS_NODE_FILE) {
        return 0U;
    }

    if (x86_64_vfs_type(&probe, "/usr/share", &type) != X86_64_VFS_RET_OK ||
        type != X86_64_VFS_NODE_DIRECTORY) {
        return 0U;
    }

    if (x86_64_vfs_type(&probe, "/etc/../bin//hello", &type) != X86_64_VFS_RET_OK ||
        type != X86_64_VFS_NODE_EXECUTABLE) {
        return 0U;
    }

    if (x86_64_vfs_stat(&probe, "/bin/./hello", &size) != X86_64_VFS_RET_OK || size <= 64ULL) {
        return 0U;
    }

    if (x86_64_vfs_type(&probe, "/bin/sysinfo", &type) != X86_64_VFS_RET_OK ||
        type != X86_64_VFS_NODE_EXECUTABLE) {
        return 0U;
    }

    if (x86_64_vfs_stat(&probe, "/bin/../bin/sysinfo", &size) != X86_64_VFS_RET_OK || size <= 64ULL) {
        return 0U;
    }

    if (x86_64_vfs_stat(&probe, "/missing", &size) != X86_64_VFS_RET_ENOENT) {
        return 0U;
    }

    if (x86_64_vfs_close(&probe, fd) != X86_64_VFS_RET_OK ||
        x86_64_vfs_open_count(&probe) != 0U) {
        return 0U;
    }

    return 1U;
}

u32 x86_64_vfs_file_count(void)
{
    return X86_64_VFS_NODE_COUNT;
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

u64 x86_64_vfs_open(struct x86_64_vfs_state *state, const char *path, u64 flags)
{
    if (state == (struct x86_64_vfs_state *)0 || path == (const char *)0 || flags != 0ULL) {
        return X86_64_VFS_RET_EINVAL;
    }

    u32 node_index = x86_64_vfs_find_node(path);
    if (node_index == X86_64_VFS_NO_NODE) {
        return X86_64_VFS_RET_ENOENT;
    }

    for (u32 slot = 0U; slot < X86_64_VFS_MAX_OPEN_FILES; ++slot) {
        if (state->fd_used[slot] == 0U) {
            state->fd_used[slot] = 1U;
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
    if (node_index >= X86_64_VFS_NODE_COUNT) {
        return X86_64_VFS_RET_EBADF;
    }

    const struct x86_64_vfs_node *node = &x86_64_vfs_nodes[node_index];
    u64 node_size = x86_64_vfs_node_size(node);
    u64 offset = state->fd_offset[slot];
    if (offset >= node_size || size == 0ULL) {
        return 0ULL;
    }

    u64 remaining = node_size - offset;
    u64 bytes = (size < remaining) ? size : remaining;
    for (u64 i = 0ULL; i < bytes; ++i) {
        buffer[i] = (char)node->data[offset + i];
    }

    state->fd_offset[slot] = offset + bytes;
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
    state->fd_node_index[slot] = X86_64_VFS_NO_NODE;
    state->fd_offset[slot] = 0ULL;
    return X86_64_VFS_RET_OK;
}

u64 x86_64_vfs_stat(const struct x86_64_vfs_state *state, const char *path, u64 *size_out)
{
    if (state == (const struct x86_64_vfs_state *)0 ||
        path == (const char *)0 ||
        size_out == (u64 *)0) {
        return X86_64_VFS_RET_EINVAL;
    }

    u32 node_index = x86_64_vfs_find_node(path);
    if (node_index == X86_64_VFS_NO_NODE) {
        return X86_64_VFS_RET_ENOENT;
    }

    *size_out = x86_64_vfs_node_size(&x86_64_vfs_nodes[node_index]);
    return X86_64_VFS_RET_OK;
}

u64 x86_64_vfs_type(const struct x86_64_vfs_state *state, const char *path, u64 *type_out)
{
    if (state == (const struct x86_64_vfs_state *)0 ||
        path == (const char *)0 ||
        type_out == (u64 *)0) {
        return X86_64_VFS_RET_EINVAL;
    }

    u32 node_index = x86_64_vfs_find_node(path);
    if (node_index == X86_64_VFS_NO_NODE) {
        return X86_64_VFS_RET_ENOENT;
    }

    *type_out = (u64)x86_64_vfs_nodes[node_index].type;
    return X86_64_VFS_RET_OK;
}

u64 x86_64_vfs_resolve(const struct x86_64_vfs_state *state, const char *path,
                       const u8 **data_out, u64 *size_out, u64 *type_out)
{
    if (state == (const struct x86_64_vfs_state *)0 ||
        state->initialized == 0U ||
        path == (const char *)0 ||
        data_out == (const u8 **)0 ||
        size_out == (u64 *)0 ||
        type_out == (u64 *)0) {
        return X86_64_VFS_RET_EINVAL;
    }

    u32 node_index = x86_64_vfs_find_node(path);
    if (node_index == X86_64_VFS_NO_NODE) {
        return X86_64_VFS_RET_ENOENT;
    }

    const struct x86_64_vfs_node *node = &x86_64_vfs_nodes[node_index];
    *data_out = node->data;
    *size_out = x86_64_vfs_node_size(node);
    *type_out = (u64)node->type;
    return X86_64_VFS_RET_OK;
}
