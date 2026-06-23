bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_GETPID 9
%define LIAM_SYSCALL_YIELD 10

section .text
global _start

_start:
    mov eax, LIAM_SYSCALL_GETPID
    syscall

    mov eax, LIAM_SYSCALL_YIELD
    syscall

    mov eax, LIAM_SYSCALL_EXIT
    xor edi, edi
    syscall

.halt:
    hlt
    jmp .halt
