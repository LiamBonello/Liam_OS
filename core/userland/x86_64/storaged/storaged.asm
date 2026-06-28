bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_OPEN 3
%define LIAM_SYSCALL_READ 4
%define LIAM_SYSCALL_CLOSE 5
%define LIAM_STDOUT 1
%define LIAM_OPEN_WRITE 1
%define LIAM_OPEN_CREATE 2
%define LIAM_OPEN_TRUNCATE 4
%define LIAM_READ_BUFFER_LEN 64

section .text
global _start

_start:
    sub rsp, LIAM_READ_BUFFER_LEN

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel title_text]
    mov edx, title_text_len
    syscall

    mov eax, LIAM_SYSCALL_OPEN
    lea rdi, [rel session_path]
    mov esi, LIAM_OPEN_WRITE | LIAM_OPEN_CREATE | LIAM_OPEN_TRUNCATE
    syscall
    cmp rax, 3
    jb .failed
    cmp rax, 6
    ja .failed
    mov r12, rax

    mov eax, LIAM_SYSCALL_WRITE
    mov rdi, r12
    lea rsi, [rel session_text]
    mov edx, session_text_len
    syscall
    cmp rax, session_text_len
    jne .close_failed

    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r12
    syscall

    mov eax, LIAM_SYSCALL_OPEN
    lea rdi, [rel session_path]
    xor esi, esi
    syscall
    cmp rax, 3
    jb .failed
    cmp rax, 6
    ja .failed
    mov r12, rax

    mov eax, LIAM_SYSCALL_READ
    mov rdi, r12
    mov rsi, rsp
    mov edx, LIAM_READ_BUFFER_LEN
    syscall
    test rax, rax
    jle .close_failed

    mov r13, rax
    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r12
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    mov rsi, rsp
    mov rdx, r13
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel ok_text]
    mov edx, ok_text_len
    syscall
    jmp .exit_success

.close_failed:
    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r12
    syscall

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
    db "Liam_OS storage service", 10
title_text_len equ $ - title_text
session_path:
    db "/tmp/session.txt", 0
session_text:
    db "session storage ok", 10
session_text_len equ $ - session_text
ok_text:
    db "storage-write ok", 10
ok_text_len equ $ - ok_text
failed_text:
    db "storage-write failed", 10
failed_text_len equ $ - failed_text
