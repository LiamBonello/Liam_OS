#ifndef LIAM_OS_IDT_H
#define LIAM_OS_IDT_H

#include "../../core/types.h"


typedef struct
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t interrupt_number;
    uint32_t error_code;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} interrupt_registers_t;

typedef void (*irq_handler_t)(interrupt_registers_t *registers);

void idt_initialize(void);
void idt_exception_handler(interrupt_registers_t *registers);
void idt_irq_handler(interrupt_registers_t *registers);
void idt_register_irq_handler(uint8_t irq, irq_handler_t handler);

#endif