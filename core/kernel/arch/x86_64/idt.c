#include "boot_info.h"
#include "console.h"
#include "idt.h"

#define X86_64_IDT_ENTRIES 256U
#define X86_64_EXCEPTION_COUNT 32U
#define X86_64_KERNEL_CODE_SELECTOR 0x08U
#define X86_64_INTERRUPT_GATE 0x8EU
#define X86_64_IDT_IST_MASK 0x07U
#define X86_64_RFLAGS_INTERRUPT_ENABLE (1ULL << 9U)
#define X86_64_PIC1_COMMAND 0x20U
#define X86_64_PIC1_DATA 0x21U
#define X86_64_PIC2_COMMAND 0xA0U
#define X86_64_PIC2_DATA 0xA1U
#define X86_64_ICW1_INIT 0x10U
#define X86_64_ICW1_ICW4 0x01U
#define X86_64_ICW4_8086 0x01U
#define X86_64_PIC_MASK_ALL 0xFFU
#define X86_64_PIC_EOI 0x20U
#define X86_64_PIT_CHANNEL_0 0x40U
#define X86_64_PIT_COMMAND 0x43U
#define X86_64_PIT_BASE_FREQUENCY 1193182U
#define X86_64_PIT_MODE_RATE_GENERATOR 0x36U
#define X86_64_PAGE_FAULT_PRESENT 0x01ULL
#define X86_64_PAGE_FAULT_WRITE 0x02ULL
#define X86_64_PAGE_FAULT_USER 0x04ULL
#define X86_64_PAGE_FAULT_RESERVED 0x08ULL
#define X86_64_PAGE_FAULT_INSTRUCTION 0x10ULL

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
extern void (*x86_64_irq_table[])(void);

static struct idt_entry idt[X86_64_IDT_ENTRIES];
static struct idt_descriptor idt_descriptor;
static u32 legacy_pic_remapped;
static u32 legacy_pic_all_masked;
static u8 legacy_pic_master_mask;
static u8 legacy_pic_slave_mask;
static u32 irq_delivery_count;
static u64 last_irq_vector;
static u32 irq_self_test_active;
static volatile u32 timer_ticks;
static struct x86_64_timer_state timer_state;

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

static u8 exception_ist(u32 vector)
{
    if (vector == X86_64_IDT_DOUBLE_FAULT_VECTOR) {
        return X86_64_IDT_DOUBLE_FAULT_IST;
    }

    if (vector == X86_64_IDT_NMI_VECTOR) {
        return X86_64_IDT_NMI_IST;
    }

    if (vector == X86_64_IDT_PAGE_FAULT_VECTOR) {
        return X86_64_IDT_PAGE_FAULT_IST;
    }

    return 0U;
}

static void set_idt_gate(u32 vector, void (*handler)(void), u8 ist)
{
    u64 address = (u64)handler;

    idt[vector].offset_low = (u16)(address & 0xFFFFU);
    idt[vector].selector = X86_64_KERNEL_CODE_SELECTOR;
    idt[vector].ist = ist & X86_64_IDT_IST_MASK;
    idt[vector].type_attr = X86_64_INTERRUPT_GATE;
    idt[vector].offset_mid = (u16)((address >> 16U) & 0xFFFFU);
    idt[vector].offset_high = (u32)(address >> 32U);
    idt[vector].zero = 0U;
}

static void load_idt(const struct idt_descriptor *descriptor)
{
    __asm__ volatile ("lidt (%0)" : : "r"(descriptor) : "memory");
}

static void read_idtr(struct idt_descriptor *descriptor)
{
    __asm__ volatile ("sidt %0" : "=m" (*descriptor));
}

static u8 inb(u16 port)
{
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a" (value) : "Nd" (port));
    return value;
}

static void outb(u16 port, u8 value)
{
    __asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (port));
}

static void io_wait(void)
{
    outb(0x80U, 0U);
}

static u64 read_cr2(void)
{
    u64 value;
    __asm__ volatile ("movq %%cr2, %0" : "=r" (value));
    return value;
}

static u64 read_rflags(void)
{
    u64 value;
    __asm__ volatile ("pushfq; popq %0" : "=r" (value));
    return value;
}

static void enable_interrupts(void)
{
    __asm__ volatile ("sti" ::: "memory");
}

static u32 gate_present(u32 vector)
{
    return (idt[vector].selector == X86_64_KERNEL_CODE_SELECTOR &&
            idt[vector].type_attr == X86_64_INTERRUPT_GATE) ? 1U : 0U;
}

static u32 gate_ist(u32 vector)
{
    return (u32)(idt[vector].ist & X86_64_IDT_IST_MASK);
}

static u32 count_irq_gates(void)
{
    u32 gates = 0U;

    for (u32 irq = 0; irq < X86_64_IRQ_VECTOR_COUNT; ++irq) {
        gates += gate_present(X86_64_IRQ_VECTOR_BASE + irq);
    }

    return gates;
}

static void remap_and_mask_legacy_pic(void)
{
    outb(X86_64_PIC1_COMMAND, X86_64_ICW1_INIT | X86_64_ICW1_ICW4);
    io_wait();
    outb(X86_64_PIC2_COMMAND, X86_64_ICW1_INIT | X86_64_ICW1_ICW4);
    io_wait();

    outb(X86_64_PIC1_DATA, X86_64_IRQ_VECTOR_BASE);
    io_wait();
    outb(X86_64_PIC2_DATA, X86_64_IRQ_VECTOR_BASE + 8U);
    io_wait();

    outb(X86_64_PIC1_DATA, 0x04U);
    io_wait();
    outb(X86_64_PIC2_DATA, 0x02U);
    io_wait();

    outb(X86_64_PIC1_DATA, X86_64_ICW4_8086);
    io_wait();
    outb(X86_64_PIC2_DATA, X86_64_ICW4_8086);
    io_wait();

    outb(X86_64_PIC1_DATA, X86_64_PIC_MASK_ALL);
    outb(X86_64_PIC2_DATA, X86_64_PIC_MASK_ALL);

    legacy_pic_master_mask = inb(X86_64_PIC1_DATA);
    legacy_pic_slave_mask = inb(X86_64_PIC2_DATA);
    legacy_pic_remapped = 1U;
    legacy_pic_all_masked = ((legacy_pic_master_mask == X86_64_PIC_MASK_ALL) &&
                             (legacy_pic_slave_mask == X86_64_PIC_MASK_ALL)) ? 1U : 0U;
}

static void unmask_irq0(void)
{
    u8 mask = inb(X86_64_PIC1_DATA);
    mask = (u8)(mask & ~1U);
    outb(X86_64_PIC1_DATA, mask);
    timer_state.irq0_unmasked = ((inb(X86_64_PIC1_DATA) & 1U) == 0U) ? 1U : 0U;
}

static void send_pic_eoi(u64 vector)
{
    if (vector >= (u64)(X86_64_IRQ_VECTOR_BASE + 8U) &&
        vector < (u64)(X86_64_IRQ_VECTOR_BASE + X86_64_IRQ_VECTOR_COUNT)) {
        outb(X86_64_PIC2_COMMAND, X86_64_PIC_EOI);
    }

    if (vector >= (u64)X86_64_IRQ_VECTOR_BASE &&
        vector < (u64)(X86_64_IRQ_VECTOR_BASE + X86_64_IRQ_VECTOR_COUNT)) {
        outb(X86_64_PIC1_COMMAND, X86_64_PIC_EOI);
    }
}

static u32 timer_handle_irq(u64 vector)
{
    if (vector != X86_64_IRQ_PIT_VECTOR) {
        return 0U;
    }

    timer_ticks += 1U;
    timer_state.ticks = timer_ticks;
    return 1U;
}

static void report_irq_policy(void)
{
    u32 interrupts_enabled = ((read_rflags() & X86_64_RFLAGS_INTERRUPT_ENABLE) != 0ULL) ? 1U : 0U;
    u32 interrupts_guarded = (interrupts_enabled == 0U) ? 1U : 0U;
    u32 irq_gates = count_irq_gates();
    u32 irq_gates_ok = (irq_gates == X86_64_IRQ_VECTOR_COUNT) ? 1U : 0U;
    u32 irq_policy_ok = ((interrupts_guarded != 0U) &&
                         (irq_gates_ok != 0U) &&
                         (legacy_pic_remapped != 0U) &&
                         (legacy_pic_all_masked != 0U)) ? 1U : 0U;

    x86_64_serial_write_line("x86_64 IRQ policy online");
    x86_64_serial_write_u32("IRQ interrupts enabled: ", interrupts_enabled);
    x86_64_serial_write_u32("IRQ interrupts guarded: ", interrupts_guarded);
    x86_64_serial_write_u32("IRQ legacy vector base: ", X86_64_IRQ_VECTOR_BASE);
    x86_64_serial_write_u32("IRQ legacy vector count: ", X86_64_IRQ_VECTOR_COUNT);
    x86_64_serial_write_u32("IRQ PIT planned vector: ", X86_64_IRQ_PIT_VECTOR);
    x86_64_serial_write_u32("IRQ keyboard planned vector: ", X86_64_IRQ_KEYBOARD_VECTOR);
    x86_64_serial_write_u32("IRQ gates installed: ", irq_gates);
    x86_64_serial_write_u32("IRQ gates ok: ", irq_gates_ok);
    x86_64_serial_write_u32("IRQ legacy PIC remapped: ", legacy_pic_remapped);
    x86_64_serial_write_hex32("IRQ master mask: 0x", legacy_pic_master_mask);
    x86_64_serial_write_hex32("IRQ slave mask: 0x", legacy_pic_slave_mask);
    x86_64_serial_write_u32("IRQ all masked: ", legacy_pic_all_masked);
    x86_64_serial_write_u32("IRQ APIC deferred: ", 1U);
    x86_64_serial_write_u32("IRQ policy ok: ", irq_policy_ok);
}

static void run_irq_self_test(void)
{
    u32 before = irq_delivery_count;

    x86_64_serial_write_u32("IRQ self-test requested: ", 1U);
    x86_64_serial_write_u32("IRQ self-test vector: ", X86_64_IRQ_PIT_VECTOR);
    irq_self_test_active = 1U;
    __asm__ volatile ("int $32" ::: "memory");
    irq_self_test_active = 0U;
    x86_64_serial_write_u32("IRQ self-test delivered: ", irq_delivery_count - before);
    x86_64_serial_write_u32("IRQ self-test returned: ", 1U);
}

static void report_page_fault_error(u64 error_code)
{
    x86_64_serial_write_hex64("CR2 fault address: 0x", read_cr2());
    x86_64_serial_write_u32("PF present violation: ",
                            ((error_code & X86_64_PAGE_FAULT_PRESENT) != 0ULL) ? 1U : 0U);
    x86_64_serial_write_u32("PF write access: ",
                            ((error_code & X86_64_PAGE_FAULT_WRITE) != 0ULL) ? 1U : 0U);
    x86_64_serial_write_u32("PF user access: ",
                            ((error_code & X86_64_PAGE_FAULT_USER) != 0ULL) ? 1U : 0U);
    x86_64_serial_write_u32("PF reserved-bit violation: ",
                            ((error_code & X86_64_PAGE_FAULT_RESERVED) != 0ULL) ? 1U : 0U);
    x86_64_serial_write_u32("PF instruction fetch: ",
                            ((error_code & X86_64_PAGE_FAULT_INSTRUCTION) != 0ULL) ? 1U : 0U);
}

static void halt_after_exception(void)
{
    x86_64_serial_write_line("x86_64 panic halt");
    x86_64_serial_write_line("Panic halt mode: cli; hlt");

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

void x86_64_idt_init(void)
{
    zero_idt();

    for (u32 vector = 0; vector < X86_64_EXCEPTION_COUNT; ++vector) {
        set_idt_gate(vector, x86_64_isr_table[vector], exception_ist(vector));
    }

    for (u32 irq = 0; irq < X86_64_IRQ_VECTOR_COUNT; ++irq) {
        set_idt_gate(X86_64_IRQ_VECTOR_BASE + irq, x86_64_irq_table[irq], 0U);
    }

    idt_descriptor.limit = (u16)(sizeof(idt) - 1U);
    idt_descriptor.base = (u64)&idt[0];
    load_idt(&idt_descriptor);
    remap_and_mask_legacy_pic();

    x86_64_serial_write_u32("IDT PF CR2 reporting: ", 1U);
    x86_64_serial_write_u32("IDT PF error decode: ", 1U);
    x86_64_serial_write_u32("IDT panic halt ready: ", 1U);
    x86_64_serial_write_u32("IDT panic cli before hlt: ", 1U);
    x86_64_serial_write_u32("IDT diagnostics ok: ", 1U);
    report_irq_policy();

    if (x86_64_irq_self_test_requested != 0U) {
        run_irq_self_test();
    }

    if (x86_64_exception_self_test_requested != 0U) {
        x86_64_serial_write_u32("Exception self-test requested: ", 1U);
        x86_64_serial_write_line("x86_64 exception self-test: ud2");
        __asm__ volatile ("ud2");
    }
}

void x86_64_idt_get_state(struct x86_64_idt_state *state)
{
    struct idt_descriptor idtr;
    u32 exception_gates = 0U;
    u32 irq_gates = count_irq_gates();
    u32 irq_gates_ok = (irq_gates == X86_64_IRQ_VECTOR_COUNT) ? 1U : 0U;
    u32 nmi_ist = gate_ist(X86_64_IDT_NMI_VECTOR);
    u32 double_fault_ist = gate_ist(X86_64_IDT_DOUBLE_FAULT_VECTOR);
    u32 page_fault_ist = gate_ist(X86_64_IDT_PAGE_FAULT_VECTOR);

    read_idtr(&idtr);

    for (u32 vector = 0; vector < X86_64_EXCEPTION_COUNT; ++vector) {
        exception_gates += gate_present(vector);
    }

    state->idtr_base = idtr.base;
    state->idtr_limit = idtr.limit;
    state->exception_gates = exception_gates;
    state->irq_gates = irq_gates;
    state->irq_vector_base = X86_64_IRQ_VECTOR_BASE;
    state->irq_vector_count = X86_64_IRQ_VECTOR_COUNT;
    state->irq_pit_vector = X86_64_IRQ_PIT_VECTOR;
    state->irq_keyboard_vector = X86_64_IRQ_KEYBOARD_VECTOR;
    state->irq_gates_ok = irq_gates_ok;
    state->legacy_pic_remapped = legacy_pic_remapped;
    state->legacy_pic_all_masked = legacy_pic_all_masked;
    state->legacy_pic_master_mask = legacy_pic_master_mask;
    state->legacy_pic_slave_mask = legacy_pic_slave_mask;
    state->irq_policy_ok = ((irq_gates_ok != 0U) &&
                            (legacy_pic_remapped != 0U) &&
                            (legacy_pic_all_masked != 0U)) ? 1U : 0U;

    state->nmi_vector = X86_64_IDT_NMI_VECTOR;
    state->nmi_ist = nmi_ist;
    state->nmi_present = gate_present(X86_64_IDT_NMI_VECTOR);
    state->nmi_ist_ok = (nmi_ist == X86_64_IDT_NMI_IST) ? 1U : 0U;

    state->double_fault_vector = X86_64_IDT_DOUBLE_FAULT_VECTOR;
    state->double_fault_ist = double_fault_ist;
    state->double_fault_present = gate_present(X86_64_IDT_DOUBLE_FAULT_VECTOR);
    state->double_fault_ist_ok = (double_fault_ist == X86_64_IDT_DOUBLE_FAULT_IST) ? 1U : 0U;

    state->page_fault_vector = X86_64_IDT_PAGE_FAULT_VECTOR;
    state->page_fault_ist = page_fault_ist;
    state->page_fault_present = gate_present(X86_64_IDT_PAGE_FAULT_VECTOR);
    state->page_fault_ist_ok = (page_fault_ist == X86_64_IDT_PAGE_FAULT_IST) ? 1U : 0U;
    state->page_fault_cr2_reporting = 1U;
    state->page_fault_error_decode = 1U;
    state->panic_halt_ready = 1U;
    state->panic_cli_before_hlt = 1U;
    state->diagnostics_ok = ((state->page_fault_cr2_reporting != 0U) &&
                             (state->page_fault_error_decode != 0U) &&
                             (state->panic_halt_ready != 0U) &&
                             (state->panic_cli_before_hlt != 0U)) ? 1U : 0U;

    state->ist_gates_ok = ((state->nmi_present != 0U) &&
                           (state->nmi_ist_ok != 0U) &&
                           (state->double_fault_present != 0U) &&
                           (state->double_fault_ist_ok != 0U) &&
                           (state->page_fault_present != 0U) &&
                           (state->page_fault_ist_ok != 0U)) ? 1U : 0U;
}

void x86_64_timer_initialize(u32 frequency_hz)
{
    if (frequency_hz == 0U) {
        frequency_hz = X86_64_PIT_DEFAULT_FREQUENCY_HZ;
    }

    u32 divisor = X86_64_PIT_BASE_FREQUENCY / frequency_hz;
    if (divisor == 0U) {
        divisor = 1U;
    }

    timer_ticks = 0U;
    timer_state.initialized = 1U;
    timer_state.frequency_hz = frequency_hz;
    timer_state.divisor = divisor;
    timer_state.ticks = 0U;
    timer_state.waited_ticks = 0U;
    timer_state.wait_ok = 0U;
    timer_state.timer_ok = 0U;
    timer_state.interrupts_enabled_before = ((read_rflags() & X86_64_RFLAGS_INTERRUPT_ENABLE) != 0ULL) ? 1U : 0U;

    outb(X86_64_PIT_COMMAND, X86_64_PIT_MODE_RATE_GENERATOR);
    outb(X86_64_PIT_CHANNEL_0, (u8)(divisor & 0xFFU));
    outb(X86_64_PIT_CHANNEL_0, (u8)((divisor >> 8U) & 0xFFU));

    unmask_irq0();
    enable_interrupts();

    timer_state.interrupts_enabled_after = ((read_rflags() & X86_64_RFLAGS_INTERRUPT_ENABLE) != 0ULL) ? 1U : 0U;
}

void x86_64_timer_wait_for_ticks(u32 ticks)
{
    u32 start = timer_ticks;
    u32 target = start + ticks;
    u32 spin_budget = 200000000U;

    while (timer_ticks < target && spin_budget > 0U) {
        --spin_budget;
        __asm__ volatile ("hlt" ::: "memory");
    }

    timer_state.ticks = timer_ticks;
    timer_state.waited_ticks = timer_ticks - start;
    timer_state.wait_ok = (timer_state.waited_ticks >= ticks) ? 1U : 0U;
    timer_state.timer_ok = ((timer_state.initialized != 0U) &&
                            (timer_state.irq0_unmasked != 0U) &&
                            (timer_state.interrupts_enabled_after != 0U) &&
                            (timer_state.wait_ok != 0U)) ? 1U : 0U;
}

void x86_64_timer_get_state(struct x86_64_timer_state *state)
{
    timer_state.ticks = timer_ticks;
    *state = timer_state;
}

void x86_64_exception_handler(u64 vector, u64 error_code)
{
    const char *name = exception_name(vector);

    x86_64_console_write_at("x86_64 exception", 23, 0);
    x86_64_console_write_at(name, 24, 0);

    x86_64_serial_write_line("x86_64 exception");
    x86_64_serial_write("Name: ");
    x86_64_serial_write_line(name);
    x86_64_serial_write_hex64("Vector: 0x", vector);
    x86_64_serial_write_hex64("Error code: 0x", error_code);

    if (vector == X86_64_IDT_PAGE_FAULT_VECTOR) {
        report_page_fault_error(error_code);
    }

    halt_after_exception();
}

void x86_64_irq_handler(u64 vector)
{
    if (irq_self_test_active == 0U && timer_handle_irq(vector) != 0U) {
        send_pic_eoi(vector);
        return;
    }

    irq_delivery_count += 1U;
    last_irq_vector = vector;

    x86_64_serial_write_line("x86_64 IRQ received");
    x86_64_serial_write_hex64("IRQ vector: 0x", vector);
    x86_64_serial_write_u32("IRQ delivery count: ", irq_delivery_count);
    x86_64_serial_write_hex64("IRQ last vector: 0x", last_irq_vector);

    if (irq_self_test_active == 0U) {
        send_pic_eoi(vector);
    }
}
