#include "boot_context.h"

void x86_64_boot_context_init(u32 magic, u32 boot_info_addr, struct x86_64_boot_context *context)
{
    x86_64_boot_info_parse(magic, boot_info_addr, &context->boot_info);
    x86_64_memory_layout_init(&context->memory_layout);
}
