#ifndef LIAM_OS_GDT_H
#define LIAM_OS_GDT_H

#define GDT_KERNEL_CODE_SELECTOR 0x08
#define GDT_KERNEL_DATA_SELECTOR 0x10
#define GDT_USER_CODE_SELECTOR   0x1B
#define GDT_USER_DATA_SELECTOR   0x23
#define GDT_TSS_SELECTOR         0x28

void gdt_initialize(void);

#endif
