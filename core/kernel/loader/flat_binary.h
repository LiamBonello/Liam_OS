#ifndef LIAM_OS_FLAT_BINARY_H
#define LIAM_OS_FLAT_BINARY_H

#include "../core/types.h"
#include "../core/status.h"
#include "../core/user_image.h"
#include "../fs/vfs.h"

typedef struct
{
    uint8_t initialized;
    uint32_t load_attempts;
    uint32_t successful_loads;
    uint32_t failed_loads;
    uint32_t last_image_size;
    uint32_t last_entry;
    uint32_t last_stack_top;
    kernel_status_t last_status;
    char last_path[USER_IMAGE_PATH_MAX];
} flat_binary_loader_info_t;

void flat_binary_loader_initialize(void);
kernel_status_t flat_binary_load_from_node(const vfs_node_t* node, user_image_t* out_image);
const flat_binary_loader_info_t* flat_binary_loader_get_info(void);

#endif
