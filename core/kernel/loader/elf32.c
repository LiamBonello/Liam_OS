#include "elf32.h"
#include "../core/string.h"
#include "../arch/i386/ring3.h"

static elf32_loader_info_t elf32_info;

void elf32_initialize(void)
{
    elf32_info.initialized = 1;

    elf32_info.validation_attempts = 0;
    elf32_info.successful_validations = 0;
    elf32_info.failed_validations = 0;

    elf32_info.load_attempts = 0;
    elf32_info.successful_loads = 0;
    elf32_info.failed_loads = 0;

    elf32_info.last_entry = 0;
    elf32_info.last_program_header_count = 0;
    elf32_info.last_file_size = 0;
    elf32_info.last_load_segment_offset = 0;
    elf32_info.last_load_segment_vaddr = 0;
    elf32_info.last_load_segment_file_size = 0;
    elf32_info.last_load_segment_memory_size = 0;

    elf32_info.last_status = KERNEL_OK;
    elf32_info.last_path[0] = '\0';
}

static kernel_status_t elf32_fail(kernel_status_t status)
{
    elf32_info.failed_validations++;
    elf32_info.last_status = status;
    return status;
}

static kernel_status_t elf32_load_fail(kernel_status_t status)
{
    elf32_info.failed_loads++;
    elf32_info.last_status = status;
    return status;
}

static const char* elf32_basename(const char* path)
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

kernel_status_t elf32_validate_from_node(const vfs_node_t* node, const elf32_header_t** out_header)
{
    if (node == 0 || out_header == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    elf32_info.validation_attempts++;
    elf32_info.last_file_size = node->size;
    elf32_info.last_entry = 0;
    elf32_info.last_program_header_count = 0;
    string_copy(elf32_info.last_path, node->path, sizeof(elf32_info.last_path));

    if (node->type != VFS_NODE_FILE)
    {
        return elf32_fail(KERNEL_ERROR_INVALID_ARGUMENT);
    }

    if (node->data == 0 || node->size < sizeof(elf32_header_t))
    {
        return elf32_fail(KERNEL_ERROR_INVALID_STATE);
    }

    const elf32_header_t* header = (const elf32_header_t*)node->data;

    if (
        header->ident[0] != ELF32_MAGIC_0 ||
        header->ident[1] != ELF32_MAGIC_1 ||
        header->ident[2] != ELF32_MAGIC_2 ||
        header->ident[3] != ELF32_MAGIC_3
    )
    {
        return elf32_fail(KERNEL_ERROR_INVALID_STATE);
    }

    if (header->ident[4] != ELF32_CLASS_32BIT)
    {
        return elf32_fail(KERNEL_ERROR_UNSUPPORTED);
    }

    if (header->ident[5] != ELF32_DATA_LITTLE_ENDIAN)
    {
        return elf32_fail(KERNEL_ERROR_UNSUPPORTED);
    }

    if (header->ident[6] != ELF32_VERSION_CURRENT)
    {
        return elf32_fail(KERNEL_ERROR_UNSUPPORTED);
    }

    if (header->type != ELF32_TYPE_EXECUTABLE)
    {
        return elf32_fail(KERNEL_ERROR_UNSUPPORTED);
    }

    if (header->machine != ELF32_MACHINE_I386)
    {
        return elf32_fail(KERNEL_ERROR_UNSUPPORTED);
    }

    if (header->version != ELF32_VERSION_CURRENT)
    {
        return elf32_fail(KERNEL_ERROR_UNSUPPORTED);
    }

    if (header->header_size != sizeof(elf32_header_t))
    {
        return elf32_fail(KERNEL_ERROR_INVALID_STATE);
    }

    if (header->program_header_count > 0)
    {
        if (header->program_header_entry_size != sizeof(elf32_program_header_t))
        {
            return elf32_fail(KERNEL_ERROR_INVALID_STATE);
        }

        uint32_t program_header_bytes =
            (uint32_t)header->program_header_count *
            (uint32_t)header->program_header_entry_size;

        if (header->program_header_offset >= node->size)
        {
            return elf32_fail(KERNEL_ERROR_INVALID_STATE);
        }

        if (program_header_bytes > node->size - header->program_header_offset)
        {
            return elf32_fail(KERNEL_ERROR_INVALID_STATE);
        }
    }

    elf32_info.successful_validations++;
    elf32_info.last_entry = header->entry;
    elf32_info.last_program_header_count = header->program_header_count;
    elf32_info.last_status = KERNEL_OK;

    *out_header = header;
    return KERNEL_OK;
}

kernel_status_t elf32_load_from_node(const vfs_node_t* node, user_image_t* out_image)
{
    if (node == 0 || out_image == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (!elf32_info.initialized)
    {
        return KERNEL_ERROR_NOT_INITIALIZED;
    }

    elf32_info.load_attempts++;
    string_copy(elf32_info.last_path, node->path, sizeof(elf32_info.last_path));

    const elf32_header_t* header = 0;
    kernel_status_t status = elf32_validate_from_node(node, &header);

    if (status != KERNEL_OK)
    {
        return elf32_load_fail(status);
    }

    if (header->program_header_count == 0)
    {
        return elf32_load_fail(KERNEL_ERROR_INVALID_STATE);
    }

    const elf32_program_header_t* selected_segment = 0;

    for (uint32_t i = 0; i < header->program_header_count; i++)
    {
        const uint8_t* program_header_base =
            ((const uint8_t*)node->data) + header->program_header_offset;

        const elf32_program_header_t* program_header =
            (const elf32_program_header_t*)(
                program_header_base + (i * header->program_header_entry_size)
            );

        if (program_header->type != ELF32_PROGRAM_TYPE_LOAD)
        {
            continue;
        }

        if (selected_segment != 0)
        {
            return elf32_load_fail(KERNEL_ERROR_UNSUPPORTED);
        }

        selected_segment = program_header;
    }

    if (selected_segment == 0)
    {
        return elf32_load_fail(KERNEL_ERROR_INVALID_STATE);
    }

    if (selected_segment->virtual_address != RING3_USER_CODE_VIRTUAL)
    {
        return elf32_load_fail(KERNEL_ERROR_UNSUPPORTED);
    }

    if (header->entry < selected_segment->virtual_address)
    {
        return elf32_load_fail(KERNEL_ERROR_INVALID_STATE);
    }

    if (header->entry >= selected_segment->virtual_address + selected_segment->memory_size)
    {
        return elf32_load_fail(KERNEL_ERROR_INVALID_STATE);
    }

    if (selected_segment->file_size == 0)
    {
        return elf32_load_fail(KERNEL_ERROR_INVALID_STATE);
    }

    if (selected_segment->file_size > selected_segment->memory_size)
    {
        return elf32_load_fail(KERNEL_ERROR_INVALID_STATE);
    }

    if (selected_segment->memory_size > RING3_USER_PAGE_SIZE)
    {
        return elf32_load_fail(KERNEL_ERROR_UNSUPPORTED);
    }

    if (selected_segment->offset >= node->size)
    {
        return elf32_load_fail(KERNEL_ERROR_INVALID_STATE);
    }

    if (selected_segment->file_size > node->size - selected_segment->offset)
    {
        return elf32_load_fail(KERNEL_ERROR_INVALID_STATE);
    }

    const uint8_t* segment_data = ((const uint8_t*)node->data) + selected_segment->offset;

    ring3_loaded_image_t loaded;
    status = ring3_load_user_image(segment_data, selected_segment->file_size, &loaded);

    if (status != KERNEL_OK)
    {
        return elf32_load_fail(status);
    }

    out_image->path = node->path;
    out_image->name = elf32_basename(node->path);
    out_image->kind = USER_IMAGE_KIND_ELF;
    out_image->entry = header->entry;
    out_image->user_stack_top = loaded.user_stack_top;
    out_image->image_size = selected_segment->memory_size;
    out_image->flags = 0;

    elf32_info.successful_loads++;
    elf32_info.last_entry = header->entry;
    elf32_info.last_program_header_count = header->program_header_count;
    elf32_info.last_file_size = node->size;
    elf32_info.last_load_segment_offset = selected_segment->offset;
    elf32_info.last_load_segment_vaddr = selected_segment->virtual_address;
    elf32_info.last_load_segment_file_size = selected_segment->file_size;
    elf32_info.last_load_segment_memory_size = selected_segment->memory_size;
    elf32_info.last_status = KERNEL_OK;

    return KERNEL_OK;
}

const elf32_loader_info_t* elf32_get_info(void)
{
    return &elf32_info;
}

const char* elf32_program_type_name(uint32_t type)
{
    switch (type)
    {
    case ELF32_PROGRAM_TYPE_NULL:
        return "null";
    case ELF32_PROGRAM_TYPE_LOAD:
        return "load";
    default:
        return "unknown";
    }
}