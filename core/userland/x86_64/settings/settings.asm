bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_DESKTOP_STATUS 13
%define LIAM_SYSCALL_INPUT_STATUS 17
%define LIAM_STDOUT 1
%define LIAM_BUFFER_LEN 1024

section .text
global _start

_start:
    sub rsp, LIAM_BUFFER_LEN

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel title_text]
    mov edx, title_text_len
    syscall

    mov eax, LIAM_SYSCALL_DESKTOP_STATUS
    mov rdi, rsp
    mov esi, LIAM_BUFFER_LEN
    syscall
    test rax, rax
    jle .input

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    mov rsi, rsp
    syscall

.input:
    mov eax, LIAM_SYSCALL_INPUT_STATUS
    mov rdi, rsp
    mov esi, LIAM_BUFFER_LEN
    syscall
    test rax, rax
    jle .ready

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    mov rsi, rsp
    syscall

.ready:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel ready_text]
    mov edx, ready_text_len
    syscall

    mov eax, LIAM_SYSCALL_EXIT
    xor edi, edi
    syscall

section .rodata
title_text:
    db "Liam_OS Settings", 10
title_text_len equ $ - title_text
ready_text:
    db "settings: ready", 10
ready_text_len equ $ - ready_text
