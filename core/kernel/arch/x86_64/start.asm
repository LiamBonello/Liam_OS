global _start

section .text
bits 64

_start:
    mov rdi, 0xB8000
    mov rsi, boot_message
    mov ah, 0x0F

.write_loop:
    lodsb
    test al, al
    jz .hang
    mov [rdi], ax
    add rdi, 2
    jmp .write_loop

.hang:
    cli
    hlt
    jmp .hang

section .rodata
boot_message:
    db 'Liam_OS x86_64 experimental kernel artifact', 0
