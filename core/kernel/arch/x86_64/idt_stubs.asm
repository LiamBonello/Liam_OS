global x86_64_isr_table
extern x86_64_exception_handler

section .text
bits 64

%macro ISR_NOERR 1
global x86_64_isr%1
x86_64_isr%1:
    push qword 0
    push qword %1
    jmp x86_64_isr_common
%endmacro

%macro ISR_ERR 1
global x86_64_isr%1
x86_64_isr%1:
    push qword %1
    jmp x86_64_isr_common
%endmacro

x86_64_isr_common:
    mov rdi, [rsp]
    mov rsi, [rsp + 8]
    and rsp, -16
    call x86_64_exception_handler

.hang:
    cli
    hlt
    jmp .hang

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR 29
ISR_ERR 30
ISR_NOERR 31

section .rodata
align 8
x86_64_isr_table:
    dq x86_64_isr0
    dq x86_64_isr1
    dq x86_64_isr2
    dq x86_64_isr3
    dq x86_64_isr4
    dq x86_64_isr5
    dq x86_64_isr6
    dq x86_64_isr7
    dq x86_64_isr8
    dq x86_64_isr9
    dq x86_64_isr10
    dq x86_64_isr11
    dq x86_64_isr12
    dq x86_64_isr13
    dq x86_64_isr14
    dq x86_64_isr15
    dq x86_64_isr16
    dq x86_64_isr17
    dq x86_64_isr18
    dq x86_64_isr19
    dq x86_64_isr20
    dq x86_64_isr21
    dq x86_64_isr22
    dq x86_64_isr23
    dq x86_64_isr24
    dq x86_64_isr25
    dq x86_64_isr26
    dq x86_64_isr27
    dq x86_64_isr28
    dq x86_64_isr29
    dq x86_64_isr30
    dq x86_64_isr31
