#include "gdt.h"

extern u64 x86_64_boot_gdt_start[];
extern u64 x86_64_boot_gdt_code[];
extern u64 x86_64_boot_gdt_data[];
extern u8 x86_64_boot_gdt_end[];

struct x86_64_gdtr_snapshot {
    u16 limit;
    u64 base;
} __attribute__((packed));

static void read_gdtr(struct x86_64_gdtr_snapshot *gdtr)
{
    __asm__ volatile ("sgdt %0" : "=m" (*gdtr));
}

static u16 read_cs(void)
{
    u16 value;
    __asm__ volatile ("mov %%cs, %0" : "=r" (value));
    return value;
}

static u16 read_ds(void)
{
    u16 value;
    __asm__ volatile ("mov %%ds, %0" : "=r" (value));
    return value;
}

static u16 read_ss(void)
{
    u16 value;
    __asm__ volatile ("mov %%ss, %0" : "=r" (value));
    return value;
}

void x86_64_gdt_state_init(struct x86_64_gdt_state *state)
{
    struct x86_64_gdtr_snapshot gdtr;
    read_gdtr(&gdtr);

    state->gdtr_base = gdtr.base;
    state->gdtr_limit = gdtr.limit;
    state->code_selector = X86_64_GDT_CODE_SELECTOR;
    state->data_selector = X86_64_GDT_DATA_SELECTOR;
    state->current_cs = read_cs();
    state->current_ds = read_ds();
    state->current_ss = read_ss();
    state->null_descriptor = x86_64_boot_gdt_start[0];
    state->code_descriptor = x86_64_boot_gdt_code[0];
    state->data_descriptor = x86_64_boot_gdt_data[0];
    state->entry_count = (u32)((u64)(x86_64_boot_gdt_end - (u8 *)x86_64_boot_gdt_start) / 8ULL);
    state->limit_ok = state->gdtr_limit == X86_64_GDT_EXPECTED_LIMIT;
    state->selectors_ok =
        state->current_cs == state->code_selector &&
        state->current_ds == state->data_selector &&
        state->current_ss == state->data_selector;
}
