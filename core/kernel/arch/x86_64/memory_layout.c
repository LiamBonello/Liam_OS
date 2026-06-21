#include "memory_layout.h"

extern u8 _x86_64_kernel_start[];
extern u8 _x86_64_kernel_end[];

void x86_64_memory_layout_init(struct x86_64_memory_layout *layout)
{
    layout->kernel_load_base = X86_64_KERNEL_LOAD_BASE;
    layout->kernel_start = (u64)_x86_64_kernel_start;
    layout->kernel_end = (u64)_x86_64_kernel_end;
    layout->kernel_size_bytes = layout->kernel_end - layout->kernel_start;
    layout->bootstrap_identity_map_bytes = X86_64_BOOTSTRAP_IDENTITY_MAP_BYTES;
    layout->boot_stack_bytes = X86_64_BOOT_STACK_BYTES;
    layout->page_size = X86_64_PAGE_SIZE;
    layout->huge_page_size = X86_64_HUGE_PAGE_SIZE;
}
