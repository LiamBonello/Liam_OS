#include "gdt.h"
#include "tss.h"
#include "../../core/log.h"
#include "../../core/types.h"

#define GDT_ACCESS_KERNEL_CODE 0x9A
#define GDT_ACCESS_KERNEL_DATA 0x92
#define GDT_ACCESS_USER_CODE   0xFA
#define GDT_ACCESS_USER_DATA   0xF2
#define GDT_ACCESS_TSS         0x89

#define GDT_GRANULARITY_CODE_DATA 0xCF
#define GDT_GRANULARITY_TSS       0x00


typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

#define GDT_ENTRIES 6

static gdt_entry_t gdt_entries[GDT_ENTRIES];
static gdt_ptr_t gdt_ptr;

extern void gdt_load(uint32_t gdt_ptr_address);

static void gdt_set_entry(
    uint32_t index,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t granularity)
{
    if (index >= GDT_ENTRIES)
    {
        return;
    }

    gdt_entries[index].base_low = (uint16_t)(base & 0xFFFF);
    gdt_entries[index].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt_entries[index].base_high = (uint8_t)((base >> 24) & 0xFF);

    gdt_entries[index].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt_entries[index].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt_entries[index].granularity |= (uint8_t)(granularity & 0xF0);

    gdt_entries[index].access = access;
}

void gdt_initialize(void)
{
    tss_initialize();

    gdt_ptr.limit = (uint16_t)(sizeof(gdt_entry_t) * GDT_ENTRIES - 1);
    gdt_ptr.base = (uint32_t)&gdt_entries;

    gdt_set_entry(0, 0, 0, 0, 0);
    gdt_set_entry(1, 0, 0xFFFFFFFF, GDT_ACCESS_KERNEL_CODE, GDT_GRANULARITY_CODE_DATA);
    gdt_set_entry(2, 0, 0xFFFFFFFF, GDT_ACCESS_KERNEL_DATA, GDT_GRANULARITY_CODE_DATA);
    gdt_set_entry(3, 0, 0xFFFFFFFF, GDT_ACCESS_USER_CODE, GDT_GRANULARITY_CODE_DATA);
    gdt_set_entry(4, 0, 0xFFFFFFFF, GDT_ACCESS_USER_DATA, GDT_GRANULARITY_CODE_DATA);
    gdt_set_entry(5, tss_get_base(), tss_get_limit(), GDT_ACCESS_TSS, GDT_GRANULARITY_TSS);

    gdt_load((uint32_t)&gdt_ptr);
    tss_load(GDT_TSS_SELECTOR);

    log_success("GDT installed");
    log_success("TSS installed");
}
