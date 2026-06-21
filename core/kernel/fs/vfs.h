#ifndef LIAM_OS_VFS_H
#define LIAM_OS_VFS_H

#include "../core/types.h"
#include "../core/status.h"

#define VFS_PATH_MAX 128
#define VFS_NAME_MAX 32
#define VFS_MAX_CHILDREN 32
#define VFS_READ_CHUNK_MAX 512

typedef enum
{
    VFS_NODE_DIRECTORY = 1,
    VFS_NODE_FILE = 2,
    VFS_NODE_DEVICE = 3
} vfs_node_type_t;

typedef struct vfs_node vfs_node_t;

typedef struct
{
    uint32_t type;
    uint32_t size;
    uint32_t mode;
    uint32_t flags;
    uint32_t child_count;
} vfs_stat_t;

typedef struct
{
    uint8_t initialized;
    uint32_t mounted_nodes;
    uint32_t lookup_attempts;
    uint32_t lookup_failures;
    uint32_t read_attempts;
    uint32_t read_failures;
    kernel_status_t last_status;
    char last_path[VFS_PATH_MAX];
} vfs_info_t;

struct vfs_node
{
    const char* name;
    const char* path;
    vfs_node_type_t type;
    const uint8_t* data;
    uint32_t size;
    uint32_t mode;
    uint32_t flags;
    const vfs_node_t* parent;
    const vfs_node_t* const* children;
    uint32_t child_count;
};

void vfs_initialize(void);
kernel_status_t vfs_mount_static_tree(const vfs_node_t* root);

kernel_status_t vfs_lookup(const char* path, const vfs_node_t** out_node);
kernel_status_t vfs_read_file(const char* path, const uint8_t** out_data, uint32_t* out_size);
kernel_status_t vfs_read_node(
    const vfs_node_t* node,
    uint32_t offset,
    uint8_t* buffer,
    uint32_t length,
    uint32_t* out_bytes_read
);
kernel_status_t vfs_stat_node(const vfs_node_t* node, vfs_stat_t* out_stat);
kernel_status_t vfs_stat_path(const char* path, vfs_stat_t* out_stat);
kernel_status_t vfs_list_directory(
    const char* path,
    const vfs_node_t* const** out_children,
    uint32_t* out_count
);

const vfs_info_t* vfs_get_info(void);
const vfs_node_t* vfs_get_root(void);
const char* vfs_node_type_name(vfs_node_type_t type);

#endif
