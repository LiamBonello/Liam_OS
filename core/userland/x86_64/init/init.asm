bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_READ 4
%define LIAM_SYSCALL_GETPID 9
%define LIAM_SYSCALL_YIELD 10

%define LIAM_STDIN 0
%define LIAM_STDOUT 1
%define LIAM_READ_BUFFER_LEN 64

section .text
global _start

_start:
    mov eax, LIAM_SYSCALL_GETPID
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel shell_banner]
    mov edx, shell_banner_len
    syscall

    mov eax, LIAM_SYSCALL_READ
    mov edi, LIAM_STDIN
    lea rsi, [rsp - LIAM_READ_BUFFER_LEN]
    mov edx, LIAM_READ_BUFFER_LEN
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
