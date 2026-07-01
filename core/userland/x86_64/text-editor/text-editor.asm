bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_OPEN 3
%define LIAM_SYSCALL_READ 4
%define LIAM_SYSCALL_CLOSE 5
%define LIAM_STDOUT 1
%define LIAM_BUFFER_LEN 512

section .text
global _start

_start:
    sub rsp, LIAM_BUFFER_LEN

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel title_text]
    mov edx, title_text_len
    syscall

    mov eax, LIAM_SYSCALL_OPEN
    lea rdi, [rel session_path]
    xor esi, esi
    syscall
    cmp rax, 3
    jb .empty
    cmp rax, 6
    ja .empty
    mov r12, rax

    mov eax, LIAM_SYSCALL_READ
    mov rdi, r12
    mov rsi, rsp
    mov edx, LIAM_BUFFER_LEN
    syscall
    mov r13, rax

    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r12
    syscall

    test r13, r13
    jle .empty

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel buffer_text]
    mov edx, buffer_text_len
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    mov rsi, rsp
    mov rdx, r13
    syscall
    jmp .ready

.empty:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel empty_text]
    mov edx, empty_text_len
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
    db "Liam_OS Text Editor", 10
title_text_len equ $ - title_text
buffer_text:
    db "editor: buffer", 10
buffer_text_len equ $ - buffer_text
empty_text:
    db "editor: empty buffer", 10
empty_text_len equ $ - empty_text
ready_text:
    db "text-editor: ready", 10
ready_text_len equ $ - ready_text
session_path:
    db "/tmp/session.txt", 0
