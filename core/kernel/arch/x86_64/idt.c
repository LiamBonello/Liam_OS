#include "console.h"
#include "idt.h"

#define X86_64_IDT_ENTRIES 256U
#define X86_64_EXCEPTION_COUNT 32U
#define X86_64_KERNEL_CODE_SELECTOR 0x08U
#define X86_64_INTERRUPT_GATE 0x8EU

struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 zero;
} __attribute__((packed));

struct idt_descriptor {
    u16 limit;
    u64 base;
} __attribute__((packed));

extern void (*x86_64_isr_table[])(void);

static struct idt_entry idt[X86_64_IDT_ENTRIES];
static struct idt_descriptor idt_descriptor;

static const char *exception_name(u64 vector)
{
    static const char *names[X86_64_EXCEPTION_COUNT] = {
        "divide error",
        "debug",
        "non-maskable interrupt",
        "breakpoint",
        "overflow",
        "bound range exceeded",
        "invalid opcode",
        "device not available",
        "double fault",
        "coprocessor segment overrun",
        "invalid TSS",
        "segment not present",
        "stack segment fault",
        "general protection fault",
        "page fault",
        "reserved",
        "x87 floating-point exception",
        "alignment check",
        "machine check",
        "SIMD floating-point exception",
        "virtualization exception",
        "control protection exception",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "hypervisor injection exception",
        "VMM communication exception",
        "security exception",
        "reserved",
    };

    if (vector < X86_64_EXCEPTION_COUNT) {
        return names[vector];
    }

    return "unknown exception";
}

static void zero_idt(void)
{
    u8 *bytes = (u8 *)idt;
    for (usize i = 0; i < sizeof(idt); ++i) {
        bytes[i] = 0U;
    }
}

static void set_idt_gate(u32 vector, void (*handler)(void))
{
    u64 address = (u64)handler;

    idt[vector].offset_low = (u16)(address & 0xFFFFU);
    idt[vector].selector = X86_64_KERNEL_CODE_SELECTOR;
    idt[vector].ist = 0U;
    idt[vector].type_attr = X86_64_INTERRUPT_GATE;
    idt[vector].offset_mid = (u16)((address >> 16U) & 0xFFFFU);
    idt[vector].offset_high = (u32)(address >> 32U);
    idt[vector].zero = 0U;
}

static void load_idt(const struct idt_descriptor *descriptor)
{
    __asm__ volatile ("lidt (%0)" : : "r"(descriptor) : "memory");
}

void x86_64_idt_init(void)
{
    zero_idt();

    for (u32 vector = 0; vector < X86_64_EXCEPTION_COUNT; ++vector) {
        set_idt_gate(vector, x86_64_isr_table[vector]);
    }

    idt_descriptor.limit = (u16)(sizeof(idt) - 1U);
    idt_descriptor.base = (u64)&idt[0];
    load_idt(&idt_descriptor);
}

void x86_64_exception_handler(u64 vector, u64 error_code)
{
    const char *name = exception_name(vector);

    x86_64_console_write_at("x86_64 exception", 21, 0);
    x86_64_console_write_at(name, 22, 0);
    x86_64_console_write_hex64(23, 0, "Vector: 0x", vector);
    x86_64_console_write_hex64(24, 0, "Error code: 0x", error_code);

    x86_64_serial_write_line("x86_64 exception");
    x86_64_serial_write("Name: ");
    x86_64_serial_write_line(name);
    x86_64_serial_write_hex64("Vector: 0x", vector);
    x86_64_serial_write_hex64("Error code: 0x", error_code);

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
