global x86_64_start
extern kernel_main_x86_64

section .text
bits 64

x86_64_start:
    call kernel_main_x86_64

.hang:
    cli
    hlt
    jmp .hang
