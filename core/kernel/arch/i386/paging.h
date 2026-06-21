#ifndef LIAM_OS_PAGING_H
#define LIAM_OS_PAGING_H

#include "../../core/types.h"


typedef struct
{
    uint8_t enabled;
    uint32_t mapped_start;
    uint32_t mapped_end;
    uint32_t mapped_bytes;
    uint32_t page_directory_address;
    uint32_t page_table_count;
} paging_info_t;

void paging_initialize(void);
const paging_info_t* paging_get_info(void);
void paging_print_info(void);

uint32_t paging_get_fault_address(void);
uint32_t paging_get_directory_address(void);

#endif