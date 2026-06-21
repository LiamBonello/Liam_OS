#include "idt.h"
#include "pic.h"
#include "paging.h"
#include "../../core/log.h"

typedef struct
{
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

#define IDT_ENTRIES 256
#define IRQ_COUNT 16
#define KERNEL_CODE_SELECTOR 0x08
#define IDT_INTERRUPT_GATE 0x8E
#define IDT_USER_TRAP_GATE 0xEF

static idt_entry_t idt_entries[IDT_ENTRIES];
static idt_ptr_t idt_ptr;
static irq_handler_t irq_handlers[IRQ_COUNT];

extern void idt_load(uint32_t idt_ptr_address);

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

extern void ring3_syscall_stub(void);

static const char *exception_name(uint32_t interrupt_number)
{
    switch (interrupt_number)
    {
    case 0:
        return "Divide by zero";
    case 1:
        return "Debug";
    case 2:
        return "Non-maskable interrupt";
    case 3:
        return "Breakpoint";
    case 4:
        return "Overflow";
    case 5:
        return "Bound range exceeded";
    case 6:
        return "Invalid opcode";
    case 7:
        return "Device not available";
    case 8:
        return "Double fault";
    case 9:
        return "Coprocessor segment overrun";
    case 10:
        return "Invalid TSS";
    case 11:
        return "Segment not present";
    case 12:
        return "Stack-segment fault";
    case 13:
        return "General protection fault";
    case 14:
        return "Page fault";
    case 16:
        return "x87 floating-point exception";
    case 17:
        return "Alignment check";
    case 18:
        return "Machine check";
    case 19:
        return "SIMD floating-point exception";
    case 20:
        return "Virtualization exception";
    case 21:
        return "Control protection exception";
    default:
        return "Reserved or unknown exception";
    }
}

static void idt_set_gate(uint32_t index, uint32_t handler_address, uint16_t selector, uint8_t flags)
{
    idt_entries[index].base_low = (uint16_t)(handler_address & 0xFFFF);
    idt_entries[index].base_high = (uint16_t)((handler_address >> 16) & 0xFFFF);
    idt_entries[index].selector = selector;
    idt_entries[index].zero = 0;
    idt_entries[index].flags = flags;
}

static void idt_clear(void)
{
    for (uint32_t i = 0; i < IDT_ENTRIES; i++)
    {
        idt_set_gate(i, 0, 0, 0);
    }

    for (uint32_t i = 0; i < IRQ_COUNT; i++)
    {
        irq_handlers[i] = 0;
    }
}

static void idt_install_exception_handlers(void)
{
    idt_set_gate(0, (uint32_t)isr0, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(1, (uint32_t)isr1, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(2, (uint32_t)isr2, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(3, (uint32_t)isr3, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(4, (uint32_t)isr4, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(5, (uint32_t)isr5, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(6, (uint32_t)isr6, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(7, (uint32_t)isr7, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(8, (uint32_t)isr8, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(9, (uint32_t)isr9, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(10, (uint32_t)isr10, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(11, (uint32_t)isr11, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(12, (uint32_t)isr12, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(13, (uint32_t)isr13, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(14, (uint32_t)isr14, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(15, (uint32_t)isr15, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(16, (uint32_t)isr16, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(17, (uint32_t)isr17, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(18, (uint32_t)isr18, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(19, (uint32_t)isr19, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(20, (uint32_t)isr20, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(21, (uint32_t)isr21, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(22, (uint32_t)isr22, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(23, (uint32_t)isr23, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(24, (uint32_t)isr24, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(25, (uint32_t)isr25, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(26, (uint32_t)isr26, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(27, (uint32_t)isr27, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(28, (uint32_t)isr28, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(29, (uint32_t)isr29, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(30, (uint32_t)isr30, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(31, (uint32_t)isr31, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
}

static void idt_install_syscall_handler(void)
{
    idt_set_gate(128, (uint32_t)ring3_syscall_stub, KERNEL_CODE_SELECTOR, IDT_USER_TRAP_GATE);
}

static void idt_install_irq_handlers(void)
{
    idt_set_gate(32, (uint32_t)irq0, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(33, (uint32_t)irq1, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(34, (uint32_t)irq2, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(35, (uint32_t)irq3, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(36, (uint32_t)irq4, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(37, (uint32_t)irq5, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(38, (uint32_t)irq6, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(39, (uint32_t)irq7, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(40, (uint32_t)irq8, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(41, (uint32_t)irq9, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(42, (uint32_t)irq10, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(43, (uint32_t)irq11, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(44, (uint32_t)irq12, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(45, (uint32_t)irq13, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(46, (uint32_t)irq14, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(47, (uint32_t)irq15, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
}

void idt_initialize(void)
{
    idt_ptr.limit = (uint16_t)(sizeof(idt_entry_t) * IDT_ENTRIES - 1);
    idt_ptr.base = (uint32_t)&idt_entries;

    idt_clear();
    idt_install_exception_handlers();
    idt_install_irq_handlers();
    idt_install_syscall_handler();

    idt_load((uint32_t)&idt_ptr);

    log_success("IDT installed");
}

void idt_register_irq_handler(uint8_t irq, irq_handler_t handler)
{
    if (irq >= IRQ_COUNT)
    {
        log_warning("Ignored invalid IRQ handler registration");
        return;
    }

    irq_handlers[irq] = handler;
}

static void idt_print_page_fault_details(uint32_t error_code)
{
    uint32_t fault_address = paging_get_fault_address();

    log_info_hex_u32("Fault address", fault_address);

    if ((error_code & 0x1) == 0)
    {
        log_info("Page fault: page not present");
    }
    else
    {
        log_info("Page fault: protection violation");
    }

    if ((error_code & 0x2) == 0)
    {
        log_info("Access type: read");
    }
    else
    {
        log_info("Access type: write");
    }

    if ((error_code & 0x4) == 0)
    {
        log_info("CPU mode: supervisor");
    }
    else
    {
        log_info("CPU mode: user");
    }

    if ((error_code & 0x8) != 0)
    {
        log_info("Reserved bit violation: yes");
    }

    if ((error_code & 0x10) != 0)
    {
        log_info("Instruction fetch fault: yes");
    }
}

void idt_exception_handler(interrupt_registers_t *registers)
{
    log_error("Unhandled CPU exception");
    log_info("Exception name follows");
    log_error(exception_name(registers->interrupt_number));
    log_info_u32("Exception vector", registers->interrupt_number);
    log_info_hex_u32("Error code", registers->error_code);

    if (registers->interrupt_number == 14)
    {
        log_info("Page fault reason follows");
        idt_print_page_fault_details(registers->error_code);
    }

    log_info_hex_u32("EIP", registers->eip);
    log_info_hex_u32("CS", registers->cs);
    log_info_hex_u32("EFLAGS", registers->eflags);

    for (;;)
    {
        __asm__ volatile("cli");
        __asm__ volatile("hlt");
    }
}

void idt_irq_handler(interrupt_registers_t *registers)
{
    uint32_t irq = registers->interrupt_number - 32;

    if (irq < IRQ_COUNT && irq_handlers[irq] != 0)
    {
        irq_handlers[irq](registers);
    }

    pic_send_eoi((uint8_t)irq);
}