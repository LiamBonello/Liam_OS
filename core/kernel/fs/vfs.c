#include "vfs.h"
#include "../core/string.h"

static const vfs_node_t* vfs_root = 0;
static vfs_info_t vfs_info;

static void vfs_set_last_status(kernel_status_t status)
{
    vfs_info.last_status = status;
}

void vfs_initialize(void)
{
    vfs_root = 0;
    vfs_info.initialized = 1;
    vfs_info.mounted_nodes = 0;
    vfs_info.lookup_attempts = 0;
    vfs_info.lookup_failures = 0;
    vfs_info.read_attempts = 0;
    vfs_info.read_failures = 0;
    vfs_info.last_status = KERNEL_OK;
    vfs_info.last_path[0] = '\0';
}

static uint32_t vfs_count_nodes_recursive(const vfs_node_t* node)
{
    if (node == 0)
    {
        return 0;
    }

    uint32_t count = 1;

    for (uint32_t i = 0; i < node->child_count; i++)
    {
        count += vfs_count_nodes_recursive(node->children[i]);
    }

    return count;
}

kernel_status_t vfs_mount_static_tree(const vfs_node_t* root)
{
    if (!vfs_info.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    if (root == 0 || root->type != VFS_NODE_DIRECTORY)
    {
        vfs_set_last_status(KERNEL_ERROR_INVALID_ARGUMENT);
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    vfs_root = root;
    vfs_info.mounted_nodes = vfs_count_nodes_recursive(root);
    vfs_set_last_status(KERNEL_OK);
    return KERNEL_OK;
}

static int vfs_path_is_root(const char* path)
{
    return path != 0 && path[0] == '/' && path[1] == '\0';
}

static kernel_status_t vfs_find_child(
    const vfs_node_t* directory,
    const char* name,
    uint32_t name_length,
    const vfs_node_t** out_node
)
{
    if (directory == 0 || name == 0 || out_node == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (directory->type != VFS_NODE_DIRECTORY)
    {
        return KERNEL_ERROR_INVALID_STATE;
    }

    for (uint32_t i = 0; i < directory->child_count; i++)
    {
        const vfs_node_t* child = directory->children[i];

        if (child == 0 || child->name == 0)
        {
            continue;
        }

        if (string_length(child->name) != name_length)
        {
            continue;
        }

        uint32_t matched = 1;

        for (uint32_t j = 0; j < name_length; j++)
        {
            if (child->name[j] != name[j])
            {
                matched = 0;
                break;
            }
        }

        if (matched)
        {
            *out_node = child;
            return KERNEL_OK;
        }
    }

    return KERNEL_ERROR_NOT_FOUND;
}

kernel_status_t vfs_lookup(const char* path, const vfs_node_t** out_node)
{
    if (!vfs_info.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    if (path == 0 || out_node == 0)
    {
        vfs_set_last_status(KERNEL_ERROR_INVALID_ARGUMENT);
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    vfs_info.lookup_attempts++;
    string_copy(vfs_info.last_path, path, VFS_PATH_MAX);

    if (vfs_root == 0)
    {
        vfs_info.lookup_failures++;
        vfs_set_last_status(KERNEL_ERROR_NOT_INITIALIZED);
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    if (path[0] != '/')
    {
        vfs_info.lookup_failures++;
        vfs_set_last_status(KERNEL_ERROR_INVALID_ARGUMENT);
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (vfs_path_is_root(path))
    {
        *out_node = vfs_root;
        vfs_set_last_status(KERNEL_OK);
        return KERNEL_OK;
    }

    const vfs_node_t* current = vfs_root;
    uint32_t index = 1;

    while (path[index] != '\0')
    {
        while (path[index] == '/')
        {
            index++;
        }

        if (path[index] == '\0')
        {
            break;
        }

        uint32_t start = index;

        while (path[index] != '\0' && path[index] != '/')
        {
            index++;
        }

        uint32_t length = index - start;
        const vfs_node_t* next = 0;
        kernel_status_t status = vfs_find_child(current, path + start, length, &next);

        if (status != KERNEL_OK)
        {
            vfs_info.lookup_failures++;
            vfs_set_last_status(status);
            return status;
        }

        current = next;
    }

    *out_node = current;
    vfs_set_last_status(KERNEL_OK);
    return KERNEL_OK;
}

kernel_status_t vfs_read_file(const char* path, const uint8_t** out_data, uint32_t* out_size)
{
    if (path == 0 || out_data == 0 || out_size == 0)
    {
        vfs_set_last_status(KERNEL_ERROR_INVALID_ARGUMENT);
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    vfs_info.read_attempts++;

    const vfs_node_t* node = 0;
    kernel_status_t status = vfs_lookup(path, &node);

    if (status != KERNEL_OK)
    {
        vfs_info.read_failures++;
        vfs_set_last_status(status);
        return status;
    }

    if (node->type != VFS_NODE_FILE)
    {
        vfs_info.read_failures++;
        vfs_set_last_status(KERNEL_ERROR_INVALID_STATE);
        return KERNEL_ERROR_INVALID_STATE;
    }

    *out_data = node->data;
    *out_size = node->size;
    vfs_set_last_status(KERNEL_OK);
    return KERNEL_OK;
}

kernel_status_t vfs_read_node(
    const vfs_node_t* node,
    uint32_t offset,
    uint8_t* buffer,
    uint32_t length,
    uint32_t* out_bytes_read
)
{
    if (out_bytes_read != 0)
    {
        *out_bytes_read = 0;
    }

    if (!vfs_info.initialized)
    {
        vfs_info.read_failures++;
        vfs_set_last_status(KERNEL_ERROR_NOT_INITIALIZED);
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    vfs_info.read_attempts++;

    if (node == 0 || buffer == 0 || out_bytes_read == 0)
    {
        vfs_info.read_failures++;
        vfs_set_last_status(KERNEL_ERROR_INVALID_ARGUMENT);
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (node->type != VFS_NODE_FILE)
    {
        vfs_info.read_failures++;
        vfs_set_last_status(KERNEL_ERROR_INVALID_STATE);
        return KERNEL_ERROR_INVALID_STATE;
    }

    if (node->data == 0 || offset >= node->size || length == 0)
    {
        *out_bytes_read = 0;
        vfs_set_last_status(KERNEL_OK);
        return KERNEL_OK;
    }

    uint32_t available = node->size - offset;
    uint32_t bytes_to_read = length;

    if (bytes_to_read > available)
    {
        bytes_to_read = available;
    }

    if (bytes_to_read > VFS_READ_CHUNK_MAX)
    {
        bytes_to_read = VFS_READ_CHUNK_MAX;
    }

    for (uint32_t i = 0; i < bytes_to_read; i++)
    {
        buffer[i] = node->data[offset + i];
    }

    *out_bytes_read = bytes_to_read;
    vfs_set_last_status(KERNEL_OK);

    return KERNEL_OK;
}

kernel_status_t vfs_stat_node(const vfs_node_t* node, vfs_stat_t* out_stat)
{
    if (node == 0 || out_stat == 0)
    {
        vfs_set_last_status(KERNEL_ERROR_INVALID_ARGUMENT);
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    out_stat->type = (uint32_t)node->type;
    out_stat->size = node->size;
    out_stat->mode = node->mode;
    out_stat->flags = node->flags;
    out_stat->child_count = node->child_count;

    vfs_set_last_status(KERNEL_OK);
    return KERNEL_OK;
}

kernel_status_t vfs_stat_path(const char* path, vfs_stat_t* out_stat)
{
    const vfs_node_t* node = 0;
    kernel_status_t status = vfs_lookup(path, &node);

    if (status != KERNEL_OK)
    {
        return status;
    }

    return vfs_stat_node(node, out_stat);
}

kernel_status_t vfs_list_directory(const char* path, const vfs_node_t* const** out_children, uint32_t* out_count)
{
    if (path == 0 || out_children == 0 || out_count == 0)
    {
        vfs_set_last_status(KERNEL_ERROR_INVALID_ARGUMENT);
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    const vfs_node_t* node = 0;
    kernel_status_t status = vfs_lookup(path, &node);

    if (status != KERNEL_OK)
    {
        return status;
    }

    if (node->type != VFS_NODE_DIRECTORY)
    {
        vfs_set_last_status(KERNEL_ERROR_INVALID_STATE);
        return KERNEL_ERROR_INVALID_STATE;
    }

    *out_children = node->children;
    *out_count = node->child_count;
    vfs_set_last_status(KERNEL_OK);
    return KERNEL_OK;
}

const vfs_info_t* vfs_get_info(void)
{
    return &vfs_info;
}

const vfs_node_t* vfs_get_root(void)
{
    return vfs_root;
}

const char* vfs_node_type_name(vfs_node_type_t type)
{
    switch (type)
    {
    case VFS_NODE_DIRECTORY:
        return "dir";
    case VFS_NODE_FILE:
        return "file";
    case VFS_NODE_DEVICE:
        return "device";
    default:
        return "unknown";
    }
}
