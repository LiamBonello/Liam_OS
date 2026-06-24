bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_GETPID 9
%define LIAM_STDOUT 1

section .text
global _start

_start:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel title_text]
    mov edx, title_text_len
    syscall

    mov eax, LIAM_SYSCALL_GETPID
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel pid_text]
    mov edx, pid_text_len
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel features_text]
    mov edx, features_text_len
    syscall

    mov eax, LIAM_SYSCALL_EXIT
    xor edi, edi
    syscall

.halt:
    hlt
    jmp .halt

section .rodata
title_text:
    db "Liam_OS sysinfo", 10
    db "arch: x86_64", 10
    db "mode: ring3 user ELF", 10
title_text_len equ $ - title_text
pid_text:
    db "pid syscall: ok", 10
pid_text_len equ $ - pid_text
features_text:
    db "features: syscall, vfs, exec", 10
features_text_len equ $ - features_text
