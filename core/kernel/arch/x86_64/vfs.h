#ifndef LIAM_OS_X86_64_VFS_H
#define LIAM_OS_X86_64_VFS_H

#include "types.h"

#define X86_64_VFS_RET_OK 0ULL
#define X86_64_VFS_RET_EFAULT 0xFFFFFFFFFFFFFFF2ULL
#define X86_64_VFS_RET_EINVAL 0xFFFFFFFFFFFFFFEAULL
#define X86_64_VFS_RET_ENOENT 0xFFFFFFFFFFFFFFFEULL
#define X86_64_VFS_RET_EBADF 0xFFFFFFFFFFFFFFF7ULL
#define X86_64_VFS_FD_BASE 3ULL
#define X86_64_VFS_MAX_OPEN_FILES 4U
#define X86_64_VFS_PATH_MAX_BYTES 64ULL
#define X86_64_VFS_NO_NODE 0xFFFFFFFFU
#define X86_64_VFS_NO_FILE X86_64_VFS_NO_NODE
#define X86_64_VFS_NODE_FILE 1U
#define X86_64_VFS_NODE_DIRECTORY 2U

struct x86_64_vfs_state {
    u32 initialized;
    u32 fd_used[X86_64_VFS_MAX_OPEN_FILES];
    u32 fd_node_index[X86_64_VFS_MAX_OPEN_FILES];
    u64 fd_offset[X86_64_VFS_MAX_OPEN_FILES];
};

void x86_64_vfs_init(struct x86_64_vfs_state *state);
u32 x86_64_vfs_ready(const struct x86_64_vfs_state *state);
u32 x86_64_vfs_file_count(void);
u32 x86_64_vfs_open_count(const struct x86_64_vfs_state *state);
u64 x86_64_vfs_open(struct x86_64_vfs_state *state, const char *path, u64 flags);
u64 x86_64_vfs_read(struct x86_64_vfs_state *state, u64 fd, char *buffer, u64 size);
u64 x86_64_vfs_close(struct x86_64_vfs_state *state, u64 fd);
u64 x86_64_vfs_stat(const struct x86_64_vfs_state *state, const char *path, u64 *size_out);
u64 x86_64_vfs_type(const struct x86_64_vfs_state *state, const char *path, u64 *type_out);

#endif
