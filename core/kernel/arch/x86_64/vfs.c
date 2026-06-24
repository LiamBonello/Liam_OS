#include "vfs.h"

struct x86_64_vfs_file {
    const char *path;
    const char *data;
    u64 size;
};

static const char x86_64_vfs_file_os_release[] =
    "NAME=Liam_OS\n"
    "ARCH=x86_64\n"
    "BUILD=dev\n";

static const char x86_64_vfs_file_version[] =
    "Liam_OS Core x86_64 dev\n";

static const char x86_64_vfs_file_help[] =
    "Available files:\n"
    "/etc/os-release\n"
    "/proc/version\n"
    "/usr/share/help.txt\n";

static const struct x86_64_vfs_file x86_64_vfs_files[] = {
    {"/etc/os-release", x86_64_vfs_file_os_release, sizeof(x86_64_vfs_file_os_release) - 1ULL},
    {"/proc/version", x86_64_vfs_file_version, sizeof(x86_64_vfs_file_version) - 1ULL},
    {"/usr/share/help.txt", x86_64_vfs_file_help, sizeof(x86_64_vfs_file_help) - 1ULL},
};

#define X86_64_VFS_FILE_COUNT ((u32)(sizeof(x86_64_vfs_files) / sizeof(x86_64_vfs_files[0])))

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

static u32 x86_64_vfs_find_file(const char *path)
{
    for (u32 i = 0U; i < X86_64_VFS_FILE_COUNT; ++i) {
        if (x86_64_vfs_path_equals(path, x86_64_vfs_files[i].path) != 0U) {
            return i;
        }
    }

    return X86_64_VFS_NO_FILE;
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
        state->fd_file_index[i] = X86_64_VFS_NO_FILE;
        state->fd_offset[i] = 0ULL;
    }
}

u32 x86_64_vfs_ready(const struct x86_64_vfs_state *state)
{
    return ((state != (const struct x86_64_vfs_state *)0) &&
            (state->initialized != 0U) &&
            (X86_64_VFS_FILE_COUNT > 0U)) ? 1U : 0U;
}

u32 x86_64_vfs_file_count(void)
{
    return X86_64_VFS_FILE_COUNT;
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

    u32 file_index = x86_64_vfs_find_file(path);
    if (file_index == X86_64_VFS_NO_FILE) {
        return X86_64_VFS_RET_ENOENT;
    }

    for (u32 slot = 0U; slot < X86_64_VFS_MAX_OPEN_FILES; ++slot) {
        if (state->fd_used[slot] == 0U) {
            state->fd_used[slot] = 1U;
            state->fd_file_index[slot] = file_index;
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

    u32 file_index = state->fd_file_index[slot];
    if (file_index >= X86_64_VFS_FILE_COUNT) {
        return X86_64_VFS_RET_EBADF;
    }

    const struct x86_64_vfs_file *file = &x86_64_vfs_files[file_index];
    u64 offset = state->fd_offset[slot];
    if (offset >= file->size || size == 0ULL) {
        return 0ULL;
    }

    u64 remaining = file->size - offset;
    u64 bytes = (size < remaining) ? size : remaining;
    for (u64 i = 0ULL; i < bytes; ++i) {
        buffer[i] = file->data[offset + i];
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
    state->fd_file_index[slot] = X86_64_VFS_NO_FILE;
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

    u32 file_index = x86_64_vfs_find_file(path);
    if (file_index == X86_64_VFS_NO_FILE) {
        return X86_64_VFS_RET_ENOENT;
    }

    *size_out = x86_64_vfs_files[file_index].size;
    return X86_64_VFS_RET_OK;
}
