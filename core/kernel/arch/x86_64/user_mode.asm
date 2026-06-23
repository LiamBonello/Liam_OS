global x86_64_user_smoke_enter
extern x86_64_user_smoke_kernel_rsp

section .text
bits 64

x86_64_user_smoke_enter:
    mov [rel x86_64_user_smoke_kernel_rsp], rsp
    push qword 0x2B
    push rsi
    pushfq
    or qword [rsp], 0x200
    push qword 0x33
    push rdi
    iretq
