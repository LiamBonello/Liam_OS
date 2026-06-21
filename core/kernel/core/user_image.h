#ifndef LIAM_OS_USER_IMAGE_H
#define LIAM_OS_USER_IMAGE_H

#include "types.h"
#include "status.h"

#define USER_IMAGE_PATH_MAX 64
#define USER_IMAGE_NAME_MAX 32

typedef enum
{
    USER_IMAGE_KIND_EMBEDDED = 1,
    USER_IMAGE_KIND_FLAT_BINARY = 2,
    USER_IMAGE_KIND_ELF = 3
} user_image_kind_t;

typedef struct
{
    const char* path;
    const char* name;
    user_image_kind_t kind;
    uint32_t entry;
    uint32_t user_stack_top;
    uint32_t image_size;
    uint32_t flags;
} user_image_t;

typedef struct
{
    uint8_t initialized;
    uint32_t registered_images;
    uint32_t resolve_attempts;
    uint32_t resolve_failures;
    kernel_status_t last_status;
    char last_path[USER_IMAGE_PATH_MAX];
} user_image_info_t;

void user_image_initialize(void);

uint32_t user_image_count(void);
kernel_status_t user_image_get_at(uint32_t index, user_image_t* out_image);
kernel_status_t user_image_resolve(const char* path, user_image_t* out_image);

const user_image_info_t* user_image_get_info(void);
const char* user_image_kind_name(user_image_kind_t kind);

#endif
