bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_GETPID 9
%define LIAM_SYSCALL_YIELD 10

section .text
global _start

_start:
    mov eax, LIAM_SYSCALL_GETPID
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    lea rdi, [rel shell_banner]
    mov esi, shell_banner_len
    syscall

    mov eax, LIAM_SYSCALL_YIELD
    syscall

    mov eax, LIAM_SYSCALL_EXIT
    xor edi, edi
    syscall

.halt:
    hlt
    jmp .halt

section .rodata
shell_banner:
    db "Liam_OS x86_64 shell online", 10
    db "$ "
shell_banner_len equ $ - shell_banner
