#include "flat_binary.h"
#include "../core/string.h"
#include "../arch/i386/ring3.h"

static flat_binary_loader_info_t flat_binary_info;

void flat_binary_loader_initialize(void)
{
    flat_binary_info.initialized = 1;
    flat_binary_info.load_attempts = 0;
    flat_binary_info.successful_loads = 0;
    flat_binary_info.failed_loads = 0;
    flat_binary_info.last_image_size = 0;
    flat_binary_info.last_entry = 0;
    flat_binary_info.last_stack_top = 0;
    flat_binary_info.last_status = KERNEL_OK;
    flat_binary_info.last_path[0] = '\0';
}

static const char* flat_binary_basename(const char* path)
{
    const char* last = path;

    if (path == 0)
    {
        return "";
    }

    for (uint32_t i = 0; path[i] != '\0'; i++)
    {
        if (path[i] == '/' && path[i + 1] != '\0')
        {
            last = &path[i + 1];
        }
    }

    return last;
}

kernel_status_t flat_binary_load_from_node(const vfs_node_t* node, user_image_t* out_image)
{
    if (!flat_binary_info.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    if (node == 0 || out_image == 0)
    {
        flat_binary_info.failed_loads++;
        flat_binary_info.last_status = KERNEL_ERROR_INVALID_ARGUMENT;
        return flat_binary_info.last_status;
    }

    flat_binary_info.load_attempts++;
    string_copy(flat_binary_info.last_path, node->path, USER_IMAGE_PATH_MAX);

    if (node->type != VFS_NODE_FILE || node->data == 0 || node->size == 0)
    {
        flat_binary_info.failed_loads++;
        flat_binary_info.last_status = KERNEL_ERROR_INVALID_STATE;
        return flat_binary_info.last_status;
    }

    ring3_loaded_image_t loaded;
    kernel_status_t status = ring3_load_user_image(node->data, node->size, &loaded);

    flat_binary_info.last_status = status;

    if (status != KERNEL_OK)
    {
        flat_binary_info.failed_loads++;
        return status;
    }

    out_image->path = node->path;
    out_image->name = flat_binary_basename(node->path);
    out_image->kind = USER_IMAGE_KIND_FLAT_BINARY;
    out_image->entry = loaded.entry;
    out_image->user_stack_top = loaded.user_stack_top;
    out_image->image_size = loaded.image_size;
    out_image->flags = 0;

    flat_binary_info.successful_loads++;
    flat_binary_info.last_image_size = loaded.image_size;
    flat_binary_info.last_entry = loaded.entry;
    flat_binary_info.last_stack_top = loaded.user_stack_top;

    return KERNEL_OK;
}

const flat_binary_loader_info_t* flat_binary_loader_get_info(void)
{
    return &flat_binary_info;
}
