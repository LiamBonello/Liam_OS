bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_INPUT_STATUS 17
%define LIAM_STDOUT 1
%define LIAM_INPUT_BUFFER_LEN 256

section .text
global _start

_start:
    sub rsp, LIAM_INPUT_BUFFER_LEN

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel title_text]
    mov edx, title_text_len
    syscall

    mov eax, LIAM_SYSCALL_INPUT_STATUS
    mov rdi, rsp
    mov esi, LIAM_INPUT_BUFFER_LEN
    syscall
    test rax, rax
    jle .failed

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    mov rsi, rsp
    syscall
    jmp .exit_success

.failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel failed_text]
    mov edx, failed_text_len
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
    db "Liam_OS input service", 10
title_text_len equ $ - title_text
failed_text:
    db "input-status failed", 10
failed_text_len equ $ - failed_text
