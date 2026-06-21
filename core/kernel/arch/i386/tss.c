#include "tss.h"
#include "gdt.h"

#define TSS_IO_MAP_DISABLED 0xFFFF

typedef struct
{
    uint32_t previous_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

static tss_entry_t tss_entry;
static tss_info_t tss_info;

extern void gdt_load_tss(uint32_t selector);

static void tss_clear_entry(void)
{
    uint8_t* bytes = (uint8_t*)&tss_entry;

    for (uint32_t i = 0; i < sizeof(tss_entry_t); i++)
    {
        bytes[i] = 0;
    }
}

void tss_initialize(void)
{
    tss_clear_entry();

    tss_entry.ss0 = GDT_KERNEL_DATA_SELECTOR;
    tss_entry.esp0 = 0;
    tss_entry.iomap_base = TSS_IO_MAP_DISABLED;

    tss_info.initialized = 1;
    tss_info.loaded = 0;
    tss_info.selector = GDT_TSS_SELECTOR;
    tss_info.ss0 = GDT_KERNEL_DATA_SELECTOR;
    tss_info.esp0 = 0;
    tss_info.base = (uint32_t)&tss_entry;
    tss_info.limit = sizeof(tss_entry_t) - 1;
    tss_info.set_kernel_stack_count = 0;
}

void tss_load(uint16_t selector)
{
    if (!tss_info.initialized)
    {
        tss_initialize();
    }

    gdt_load_tss(selector);

    tss_info.loaded = 1;
    tss_info.selector = selector;
}

void tss_set_kernel_stack(uint32_t stack_top)
{
    if (!tss_info.initialized)
    {
        tss_initialize();
    }

    tss_entry.esp0 = stack_top;
    tss_entry.ss0 = GDT_KERNEL_DATA_SELECTOR;

    tss_info.esp0 = stack_top;
    tss_info.ss0 = GDT_KERNEL_DATA_SELECTOR;
    tss_info.set_kernel_stack_count++;
}

uint32_t tss_get_base(void)
{
    if (!tss_info.initialized)
    {
        tss_initialize();
    }

    return tss_info.base;
}

uint32_t tss_get_limit(void)
{
    if (!tss_info.initialized)
    {
        tss_initialize();
    }

    return tss_info.limit;
}

const tss_info_t* tss_get_info(void)
{
    return &tss_info;
}
