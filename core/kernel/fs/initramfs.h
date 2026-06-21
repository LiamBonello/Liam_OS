#ifndef LIAM_OS_INITRAMFS_H
#define LIAM_OS_INITRAMFS_H

#include "vfs.h"
#include "../core/status.h"

typedef struct
{
    uint8_t initialized;
    kernel_status_t last_status;
    uint32_t node_count;
    uint32_t mount_count;
} initramfs_info_t;

void initramfs_initialize(void);
kernel_status_t initramfs_mount(void);
const initramfs_info_t* initramfs_get_info(void);

#endif
