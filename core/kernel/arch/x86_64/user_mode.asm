global x86_64_user_mode_enter_init
global x86_64_user_smoke_enter
global x86_64_user_mode_exit_to_kernel
extern x86_64_user_mode_kernel_rsp
extern x86_64_user_mode_kernel_cr3

section .text
bits 64

x86_64_user_mode_enter_init:
x86_64_user_smoke_enter:
    mov [rel x86_64_user_mode_kernel_rsp], rsp
    push qword 0x2B
    push rsi
    pushfq
    or qword [rsp], 0x200
    push qword 0x33
    push rdi
    iretq

x86_64_user_mode_exit_to_kernel:
    mov rax, [rel x86_64_user_mode_kernel_cr3]
    test rax, rax
    jz .restore_kernel_stack
    mov cr3, rax
    mov qword [rel x86_64_user_mode_kernel_cr3], 0

.restore_kernel_stack:
    mov rsp, [rel x86_64_user_mode_kernel_rsp]
    ret
