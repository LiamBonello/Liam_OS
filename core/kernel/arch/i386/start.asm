global _start
extern kernel_main

section .multiboot
align 4

MBALIGN  equ 1 << 0
MEMINFO  equ 1 << 1
FLAGS    equ MBALIGN | MEMINFO
MAGIC    equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

dd MAGIC
dd FLAGS
dd CHECKSUM

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text
bits 32

_start:
    cli
    mov esp, stack_top
    mov ebp, 0

    ; GRUB passes:
    ; eax = multiboot magic
    ; ebx = multiboot information pointer
    push ebx
    push eax
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang