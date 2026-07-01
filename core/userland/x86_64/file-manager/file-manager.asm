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
    lea rdi, [rel files_path]
    xor esi, esi
    syscall
    cmp rax, 3
    jb .failed
    cmp rax, 6
    ja .failed
    mov r12, rax

.read_loop:
    mov eax, LIAM_SYSCALL_READ
    mov rdi, r12
    mov rsi, rsp
    mov edx, LIAM_BUFFER_LEN
    syscall
    test rax, rax
    jz .close_ok
    cmp rax, LIAM_BUFFER_LEN
    ja .close_failed

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    mov rsi, rsp
    syscall
    jmp .read_loop

.close_ok:
    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r12
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel ready_text]
    mov edx, ready_text_len
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

section .rodata
title_text:
    db "Liam_OS File Manager", 10
title_text_len equ $ - title_text
ready_text:
    db "file-manager: ready", 10
ready_text_len equ $ - ready_text
failed_text:
    db "file-manager: file index unavailable", 10
failed_text_len equ $ - failed_text
files_path:
    db "/usr/share/files.txt", 0
