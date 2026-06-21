#ifndef LIAM_OS_ELF32_H
#define LIAM_OS_ELF32_H

#include "../core/types.h"
#include "../core/status.h"
#include "../core/user_image.h"
#include "../fs/vfs.h"

#define ELF32_MAGIC_0 0x7F
#define ELF32_MAGIC_1 'E'
#define ELF32_MAGIC_2 'L'
#define ELF32_MAGIC_3 'F'

#define ELF32_CLASS_32BIT 1
#define ELF32_DATA_LITTLE_ENDIAN 1
#define ELF32_VERSION_CURRENT 1

#define ELF32_TYPE_EXECUTABLE 2
#define ELF32_MACHINE_I386 3

#define ELF32_PROGRAM_TYPE_NULL 0
#define ELF32_PROGRAM_TYPE_LOAD 1

typedef struct
{
    uint8_t ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t program_header_offset;
    uint32_t section_header_offset;
    uint32_t flags;
    uint16_t header_size;
    uint16_t program_header_entry_size;
    uint16_t program_header_count;
    uint16_t section_header_entry_size;
    uint16_t section_header_count;
    uint16_t section_name_string_index;
} elf32_header_t;

typedef struct
{
    uint32_t type;
    uint32_t offset;
    uint32_t virtual_address;
    uint32_t physical_address;
    uint32_t file_size;
    uint32_t memory_size;
    uint32_t flags;
    uint32_t alignment;
} elf32_program_header_t;

typedef struct
{
    uint8_t initialized;

    uint32_t validation_attempts;
    uint32_t successful_validations;
    uint32_t failed_validations;

    uint32_t load_attempts;
    uint32_t successful_loads;
    uint32_t failed_loads;

    uint32_t last_entry;
    uint32_t last_program_header_count;
    uint32_t last_file_size;
    uint32_t last_load_segment_offset;
    uint32_t last_load_segment_vaddr;
    uint32_t last_load_segment_file_size;
    uint32_t last_load_segment_memory_size;

    kernel_status_t last_status;
    char last_path[128];
} elf32_loader_info_t;

void elf32_initialize(void);
kernel_status_t elf32_validate_from_node(const vfs_node_t* node, const elf32_header_t** out_header);
kernel_status_t elf32_load_from_node(const vfs_node_t* node, user_image_t* out_image);
const elf32_loader_info_t* elf32_get_info(void);
const char* elf32_program_type_name(uint32_t type);

#endif