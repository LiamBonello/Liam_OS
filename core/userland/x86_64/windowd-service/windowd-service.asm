bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_YIELD 10
%define LIAM_SYSCALL_DESKTOP_STATUS 13
%define LIAM_SYSCALL_WINDOW_PRESENT 14
%define LIAM_STDOUT 1
%define LIAM_STATUS_BUFFER_LEN 1024

section .text
global _start

_start:
    sub rsp, LIAM_STATUS_BUFFER_LEN

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel title_text]
    mov edx, title_text_len
    syscall

    mov eax, LIAM_SYSCALL_DESKTOP_STATUS
    mov rdi, rsp
    mov esi, LIAM_STATUS_BUFFER_LEN
    syscall
    test rax, rax
    jle .failed

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    mov rsi, rsp
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel ready_text]
    mov edx, ready_text_len
    syscall

.service_loop:
    mov eax, LIAM_SYSCALL_WINDOW_PRESENT
    syscall

    mov eax, LIAM_SYSCALL_YIELD
    syscall

    jmp .service_loop

.failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel failed_text]
    mov edx, failed_text_len
    syscall

    mov eax, LIAM_SYSCALL_EXIT
    mov edi, 1
    syscall

.halt:
    hlt
    jmp .halt

section .rodata
title_text:
    db "Liam_OS window service daemon", 10
title_text_len equ $ - title_text

ready_text:
    db "windowd-service ready", 10
ready_text_len equ $ - ready_text

failed_text:
    db "windowd-service failed", 10
failed_text_len equ $ - failed_text