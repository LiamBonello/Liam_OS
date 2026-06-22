#include "console.h"
#include "gdt.h"
#include "tss.h"

#define X86_64_GDT_NULL_DESCRIPTOR 0x0000000000000000ULL
#define X86_64_GDT_CODE_DESCRIPTOR 0x00209A0000000000ULL
#define X86_64_GDT_DATA_DESCRIPTOR 0x0000920000000000ULL
#define X86_64_GDT_USER_DATA_DESCRIPTOR 0x0000F20000000000ULL
#define X86_64_GDT_USER_CODE_DESCRIPTOR 0x0020FA0000000000ULL
#define X86_64_GDT_TSS_TYPE_PRESENT 0x89ULL

struct x86_64_gdtr_snapshot {
    u16 limit;
    u64 base;
} __attribute__((packed));

static u64 maintained_gdt[X86_64_GDT_ENTRY_COUNT] __attribute__((aligned(16)));
static struct x86_64_gdtr_snapshot maintained_gdtr;

static void read_gdtr(struct x86_64_gdtr_snapshot *gdtr)
{
    __asm__ volatile ("sgdt %0" : "=m" (*gdtr));
}

static u16 read_cs(void)
{
    u16 value;
    __asm__ volatile ("movw %%cs, %0" : "=rm" (value));
    return value;
}

static u16 read_ds(void)
{
    u16 value;
    __asm__ volatile ("movw %%ds, %0" : "=rm" (value));
    return value;
}

static u16 read_ss(void)
{
    u16 value;
    __asm__ volatile ("movw %%ss, %0" : "=rm" (value));
    return value;
}

static u16 read_tr(void)
{
    u16 value;
    __asm__ volatile ("str %0" : "=rm" (value));
    return value;
}

static void write_tss_descriptor(u64 base, u32 limit)
{
    u64 low = 0ULL;
    u64 high = 0ULL;

    low |= (u64)(limit & 0xFFFFU);
    low |= (base & 0xFFFFFFULL) << 16;
    low |= X86_64_GDT_TSS_TYPE_PRESENT << 40;
    low |= ((u64)((limit >> 16) & 0xFU)) << 48;
    low |= ((base >> 24) & 0xFFULL) << 56;
    high = base >> 32;

    maintained_gdt[3] = low;
    maintained_gdt[4] = high;
}

static void build_maintained_gdt(const struct x86_64_tss_state *tss_state)
{
    maintained_gdt[0] = X86_64_GDT_NULL_DESCRIPTOR;
    maintained_gdt[1] = X86_64_GDT_CODE_DESCRIPTOR;
    maintained_gdt[2] = X86_64_GDT_DATA_DESCRIPTOR;
    write_tss_descriptor(tss_state->tss_base, tss_state->tss_limit);
    maintained_gdt[5] = X86_64_GDT_USER_DATA_DESCRIPTOR;
    maintained_gdt[6] = X86_64_GDT_USER_CODE_DESCRIPTOR;

    maintained_gdtr.limit = X86_64_GDT_EXPECTED_LIMIT;
    maintained_gdtr.base = (u64)&maintained_gdt[0];
}

static void load_gdt_and_segments(void)
{
    u16 data_selector = X86_64_GDT_DATA_SELECTOR;
    u64 code_selector = X86_64_GDT_CODE_SELECTOR;

    __asm__ volatile ("lgdt %0" : : "m" (maintained_gdtr) : "memory");
    __asm__ volatile ("movw %0, %%ds" : : "rm" (data_selector) : "memory");
    __asm__ volatile ("movw %0, %%es" : : "rm" (data_selector) : "memory");
    __asm__ volatile ("movw %0, %%ss" : : "rm" (data_selector) : "memory");
    __asm__ volatile (
        "pushq %0\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        :
        : "r" (code_selector)
        : "rax", "memory");
}

static void load_task_register(void)
{
    u16 tss_selector = X86_64_GDT_TSS_SELECTOR;
    __asm__ volatile ("ltr %0" : : "rm" (tss_selector) : "memory");
}

static void capture_state(struct x86_64_gdt_state *state)
{
    struct x86_64_gdtr_snapshot gdtr;
    read_gdtr(&gdtr);

    state->gdtr_base = gdtr.base;
    state->gdtr_limit = gdtr.limit;
    state->code_selector = X86_64_GDT_CODE_SELECTOR;
    state->data_selector = X86_64_GDT_DATA_SELECTOR;
    state->tss_selector = X86_64_GDT_TSS_SELECTOR;
    state->user_data_selector = X86_64_GDT_USER_DATA_SELECTOR;
    state->user_code_selector = X86_64_GDT_USER_CODE_SELECTOR;
    state->syscall_user_selector_base = X86_64_GDT_SYSCALL_USER_SELECTOR_BASE;
    state->current_cs = read_cs();
    state->current_ds = read_ds();
    state->current_ss = read_ss();
    state->current_tr = read_tr();
    state->null_descriptor = maintained_gdt[0];
    state->code_descriptor = maintained_gdt[1];
    state->data_descriptor = maintained_gdt[2];
    state->tss_descriptor_low = maintained_gdt[3];
    state->tss_descriptor_high = maintained_gdt[4];
    state->user_data_descriptor = maintained_gdt[5];
    state->user_code_descriptor = maintained_gdt[6];
    state->entry_count = X86_64_GDT_ENTRY_COUNT;
    state->limit_ok = state->gdtr_limit == X86_64_GDT_EXPECTED_LIMIT;
    state->selectors_ok =
        state->current_cs == state->code_selector &&
        state->current_ds == state->data_selector &&
        state->current_ss == state->data_selector;
    state->user_selectors_ok =
        state->user_data_descriptor == X86_64_GDT_USER_DATA_DESCRIPTOR &&
        state->user_code_descriptor == X86_64_GDT_USER_CODE_DESCRIPTOR &&
        state->user_data_selector == 0x2BU &&
        state->user_code_selector == 0x33U &&
        state->syscall_user_selector_base == 0x20U;
    state->tss_loaded = state->current_tr == state->tss_selector;
}

static void report_user_selectors(const struct x86_64_gdt_state *state)
{
    x86_64_serial_write_line("x86_64 user GDT selectors online");
    x86_64_serial_write_u32("GDT user data selector: ", state->user_data_selector);
    x86_64_serial_write_u32("GDT user code selector: ", state->user_code_selector);
    x86_64_serial_write_u32("GDT syscall user base: ", state->syscall_user_selector_base);
    x86_64_serial_write_hex64("GDT user data descriptor: 0x", state->user_data_descriptor);
    x86_64_serial_write_hex64("GDT user code descriptor: 0x", state->user_code_descriptor);
    x86_64_serial_write_u32("GDT user selectors ok: ", state->user_selectors_ok);
}

void x86_64_gdt_state_init(struct x86_64_gdt_state *state)
{
    capture_state(state);
}

void x86_64_gdt_load_tss(const struct x86_64_tss_state *tss_state, struct x86_64_gdt_state *state)
{
    build_maintained_gdt(tss_state);
    load_gdt_and_segments();
    load_task_register();
    capture_state(state);
    report_user_selectors(state);
}
