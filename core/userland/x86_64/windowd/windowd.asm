bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
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
    jle .present

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    mov rsi, rsp
    syscall

.present:
    mov eax, LIAM_SYSCALL_WINDOW_PRESENT
    syscall
    cmp rax, 1
    jne .present_failed

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel present_ok_text]
    mov edx, present_ok_text_len
    syscall
    jmp .exit_success

.present_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel present_failed_text]
    mov edx, present_failed_text_len
    syscall

.exit_success:
    mov eax, LIAM_SYSCALL_EXIT
    xor edi, edi
    syscall

.halt:
    hlt
    jmp .halt

section .rodata
title_text:
    db "Liam_OS window service", 10
title_text_len equ $ - title_text
present_ok_text:
    db "window-present ok", 10
present_ok_text_len equ $ - present_ok_text
present_failed_text:
    db "window-present failed", 10
present_failed_text_len equ $ - present_failed_text
