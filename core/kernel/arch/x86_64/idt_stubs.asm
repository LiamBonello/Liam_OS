bits 64

global x86_64_idt_load
global x86_64_isr_stub_table
global x86_64_irq_stub_table
global x86_64_isr_table
global x86_64_irq_table
global x86_64_syscall_entry_stub

extern x86_64_exception_handler
extern x86_64_irq_handler
extern x86_64_user_mode_exit_to_kernel
extern x86_64_user_mode_kernel_cr3
extern x86_64_user_mode_syscall_entry

%macro ISR_NOERR 1
global x86_64_isr_stub_%1
x86_64_isr_stub_%1:
    push qword 0
    push qword %1
    jmp x86_64_isr_common
%endmacro

%macro ISR_ERR 1
global x86_64_isr_stub_%1
x86_64_isr_stub_%1:
    push qword %1
    jmp x86_64_isr_common
%endmacro

x86_64_idt_load:
    lidt [rdi]
    ret

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
ISR_NOERR 21
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

x86_64_isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, [rsp + 120]
    mov rsi, [rsp + 128]
    call x86_64_exception_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    iretq

%macro IRQ_STUB 1
global x86_64_irq_stub_%1
x86_64_irq_stub_%1:
    push qword 0
    push qword (32 + %1)
    jmp x86_64_irq_common
%endmacro

IRQ_STUB 0
IRQ_STUB 1
IRQ_STUB 2
IRQ_STUB 3
IRQ_STUB 4
IRQ_STUB 5
IRQ_STUB 6
IRQ_STUB 7
IRQ_STUB 8
IRQ_STUB 9
IRQ_STUB 10
IRQ_STUB 11
IRQ_STUB 12
IRQ_STUB 13
IRQ_STUB 14
IRQ_STUB 15

x86_64_irq_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, [rsp + 120]
    call x86_64_irq_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    iretq

x86_64_syscall_entry_stub:
    push rcx
    push r11
    sub rsp, 96

    mov [rsp + 0], rax
    mov [rsp + 8], rdi
    mov [rsp + 16], rsi
    mov [rsp + 24], rdx
    mov [rsp + 32], r10
    mov [rsp + 40], r8
    mov [rsp + 48], r9
    mov rax, [rsp + 104]
    mov [rsp + 56], rax
    mov rax, [rsp + 96]
    mov [rsp + 64], rax
    mov qword [rsp + 72], 0
    mov dword [rsp + 80], 0
    mov dword [rsp + 84], 0
    mov qword [rsp + 88], 0

    mov rdi, rsp
    call x86_64_user_mode_syscall_entry
    cmp dword [rsp + 80], 1
    je .exit_to_kernel

    xor r10d, r10d
    xor r9d, r9d
    mov r8, [rsp + 88]
    cmp dword [rsp + 84], 1
    jne .check_exec_stack_reset
    cmp r8, 0
    je .check_exec_stack_reset
    mov r9d, 1

.check_exec_stack_reset:
    cmp qword [rsp + 0], 8
    jne .restore_user_return
    cmp qword [rsp + 72], 0
    jne .restore_user_return
    mov r10d, 1

.restore_user_return:
    mov rcx, [rsp + 56]
    mov [rsp + 104], rcx
    mov r11, [rsp + 64]
    mov [rsp + 96], r11

    add rsp, 96
    pop r11
    pop rcx
    cmp r10d, 1
    jne .maybe_switch_cr3
    mov rsp, 0x000000007FFFFFF0

.maybe_switch_cr3:
    cmp r9d, 1
    jne .sysret_to_user
    mov rax, cr3
    mov [rel x86_64_user_mode_kernel_cr3], rax
    mov cr3, r8

.sysret_to_user:
    o64 sysret

.exit_to_kernel:
    add rsp, 96
    add rsp, 16
    jmp x86_64_user_mode_exit_to_kernel

section .rodata
align 8
x86_64_isr_stub_table:
x86_64_isr_table:
%assign i 0
%rep 32
    dq x86_64_isr_stub_%+i
%assign i i+1
%endrep

align 8
x86_64_irq_stub_table:
x86_64_irq_table:
%assign i 0
%rep 16
    dq x86_64_irq_stub_%+i
%assign i i+1
%endrep
