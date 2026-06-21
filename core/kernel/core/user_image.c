#include "user_image.h"
#include "string.h"
#include "../fs/vfs.h"
#include "../loader/flat_binary.h"
#include "../loader/elf32.h"
#include "../arch/i386/ring3.h"

typedef struct
{
    const char* path;
    const char* name;
    user_image_kind_t kind;
} registered_user_image_t;

static const registered_user_image_t registered_images[] = {
    {
        "/sbin/init",
        "init",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/hello",
        "hello",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/hello-elf",
        "hello-elf",
        USER_IMAGE_KIND_ELF
    },
    {
        "/bin/syscheck",
        "syscheck",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/sysbadptr",
        "sysbadptr",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/pid",
        "pid",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/ticks",
        "ticks",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/about",
        "about",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/os-release",
        "os-release",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/cat",
        "cat",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/clear",
        "clear",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/help",
        "help",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/uptime",
        "uptime",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/args",
        "args",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/echo",
        "echo",
        USER_IMAGE_KIND_FLAT_BINARY
    },
    {
        "/bin/sh",
        "sh",
        USER_IMAGE_KIND_FLAT_BINARY
    }
};

static user_image_info_t user_image_info;

void user_image_initialize(void)
{
    user_image_info.initialized = 1;
    user_image_info.registered_images =
        sizeof(registered_images) / sizeof(registered_images[0]);
    user_image_info.resolve_attempts = 0;
    user_image_info.resolve_failures = 0;
    user_image_info.last_status = KERNEL_OK;
    user_image_info.last_path[0] = '\0';
}

uint32_t user_image_count(void)
{
    return sizeof(registered_images) / sizeof(registered_images[0]);
}

static kernel_status_t user_image_resolve_registered_node(
    const registered_user_image_t* registration,
    const vfs_node_t** out_node
)
{
    if (registration == 0 || out_node == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    const vfs_node_t* node = 0;
    kernel_status_t status = vfs_lookup(registration->path, &node);

    if (status != KERNEL_OK)
    {
        return status;
    }

    if (node == 0 || node->type != VFS_NODE_FILE)
    {
        return KERNEL_ERROR_INVALID_STATE;
    }

    *out_node = node;
    return KERNEL_OK;
}

static kernel_status_t user_image_load_registered(
    const registered_user_image_t* registration,
    user_image_t* out_image
)
{
    if (registration == 0 || out_image == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    const vfs_node_t* node = 0;
    kernel_status_t status = user_image_resolve_registered_node(registration, &node);

    if (status != KERNEL_OK)
    {
        return status;
    }

    switch (registration->kind)
    {
    case USER_IMAGE_KIND_FLAT_BINARY:
        return flat_binary_load_from_node(node, out_image);

    case USER_IMAGE_KIND_ELF:
        return elf32_load_from_node(node, out_image);

    case USER_IMAGE_KIND_EMBEDDED:
    default:
        return KERNEL_ERROR_UNSUPPORTED;
    }
}

kernel_status_t user_image_get_at(uint32_t index, user_image_t* out_image)
{
    if (out_image == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (index >= user_image_count())
    {
        return KERNEL_ERROR_NOT_FOUND;
    }

    const registered_user_image_t* registration = &registered_images[index];
    const vfs_node_t* node = 0;
    kernel_status_t status = user_image_resolve_registered_node(registration, &node);

    if (status != KERNEL_OK)
    {
        user_image_info.last_status = status;
        return status;
    }

    out_image->path = registration->path;
    out_image->name = registration->name;
    out_image->kind = registration->kind;
    out_image->entry = 0;
    out_image->user_stack_top = 0;
    out_image->image_size = node->size;
    out_image->flags = 0;

    user_image_info.last_status = KERNEL_OK;
    return KERNEL_OK;
}

kernel_status_t user_image_lookup(const char* path, user_image_t* out_image)
{
    if (path == 0 || out_image == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    user_image_info.resolve_attempts++;
    string_copy(user_image_info.last_path, path, USER_IMAGE_PATH_MAX);

    for (uint32_t i = 0; i < user_image_count(); i++)
    {
        if (!string_equals(path, registered_images[i].path))
        {
            continue;
        }

        const registered_user_image_t* registration = &registered_images[i];
        const vfs_node_t* node = 0;
        kernel_status_t status = user_image_resolve_registered_node(registration, &node);

        if (status != KERNEL_OK)
        {
            user_image_info.resolve_failures++;
            user_image_info.last_status = status;
            return status;
        }

        out_image->path = registration->path;
        out_image->name = registration->name;
        out_image->kind = registration->kind;
        out_image->entry = RING3_USER_CODE_VIRTUAL;
        out_image->user_stack_top = RING3_USER_STACK_VIRTUAL;
        out_image->image_size = node->size;
        out_image->flags = 0;

        user_image_info.last_status = KERNEL_OK;
        return KERNEL_OK;
    }

    user_image_info.resolve_failures++;
    user_image_info.last_status = KERNEL_ERROR_NOT_FOUND;
    return KERNEL_ERROR_NOT_FOUND;
}

kernel_status_t user_image_resolve(const char* path, user_image_t* out_image)
{
    if (path == 0 || out_image == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    user_image_info.resolve_attempts++;
    string_copy(user_image_info.last_path, path, USER_IMAGE_PATH_MAX);

    for (uint32_t i = 0; i < user_image_count(); i++)
    {
        if (string_equals(path, registered_images[i].path))
        {
            kernel_status_t status = user_image_load_registered(
                &registered_images[i],
                out_image
            );

            user_image_info.last_status = status;

            if (status != KERNEL_OK)
            {
                user_image_info.resolve_failures++;
            }

            return status;
        }
    }

    user_image_info.resolve_failures++;
    user_image_info.last_status = KERNEL_ERROR_NOT_FOUND;
    return KERNEL_ERROR_NOT_FOUND;
}

const user_image_info_t* user_image_get_info(void)
{
    return &user_image_info;
}

const char* user_image_kind_name(user_image_kind_t kind)
{
    switch (kind)
    {
    case USER_IMAGE_KIND_EMBEDDED:
        return "embedded";
    case USER_IMAGE_KIND_FLAT_BINARY:
        return "flat";
    case USER_IMAGE_KIND_ELF:
        return "elf";
    default:
        return "unknown";
    }
}
