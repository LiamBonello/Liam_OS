bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_OPEN 3
%define LIAM_SYSCALL_READ 4
%define LIAM_SYSCALL_CLOSE 5
%define LIAM_SYSCALL_STAT 6
%define LIAM_SYSCALL_MKDIR 20
%define LIAM_SYSCALL_UNLINK 21
%define LIAM_RET_OK 0
%define LIAM_RET_ENOENT 0xfffffffffffffffe
%define LIAM_STDOUT 1
%define LIAM_OPEN_WRITE 1
%define LIAM_OPEN_CREATE 2
%define LIAM_OPEN_TRUNCATE 4
%define LIAM_READ_BUFFER_LEN 96

section .text
global _start

_start:
    sub rsp, LIAM_READ_BUFFER_LEN + 8

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
    cmp rax, 10
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
    cmp rax, 10
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
    lea rsi, [rel session_ok_text]
    mov edx, session_ok_text_len
    syscall

    mov eax, LIAM_SYSCALL_MKDIR
    lea rdi, [rel config_dir]
    syscall
    cmp rax, LIAM_RET_OK
    jne .failed

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel mkdir_ok_text]
    mov edx, mkdir_ok_text_len
    syscall

    mov eax, LIAM_SYSCALL_OPEN
    lea rdi, [rel config_file]
    mov esi, LIAM_OPEN_WRITE | LIAM_OPEN_CREATE | LIAM_OPEN_TRUNCATE
    syscall
    cmp rax, 3
    jb .failed
    cmp rax, 10
    ja .failed
    mov r12, rax

    mov eax, LIAM_SYSCALL_WRITE
    mov rdi, r12
    lea rsi, [rel config_text]
    mov edx, config_text_len
    syscall
    cmp rax, config_text_len
    jne .close_failed

    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r12
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel create_ok_text]
    mov edx, create_ok_text_len
    syscall

    mov eax, LIAM_SYSCALL_OPEN
    lea rdi, [rel config_file]
    xor esi, esi
    syscall
    cmp rax, 3
    jb .failed
    cmp rax, 10
    ja .failed
    mov r12, rax

    mov eax, LIAM_SYSCALL_READ
    mov rdi, r12
    mov rsi, rsp
    mov edx, LIAM_READ_BUFFER_LEN
    syscall
    cmp rax, config_text_len
    jne .close_failed

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
    lea rsi, [rel read_ok_text]
    mov edx, read_ok_text_len
    syscall

    mov eax, LIAM_SYSCALL_UNLINK
    lea rdi, [rel config_file]
    syscall
    cmp rax, LIAM_RET_OK
    jne .failed

    mov eax, LIAM_SYSCALL_STAT
    lea rdi, [rel config_file]
    lea rsi, [rsp + LIAM_READ_BUFFER_LEN]
    syscall
    cmp rax, LIAM_RET_ENOENT
    jne .failed

    mov eax, LIAM_SYSCALL_UNLINK
    lea rdi, [rel config_dir]
    syscall
    cmp rax, LIAM_RET_OK
    jne .failed

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel delete_ok_text]
    mov edx, delete_ok_text_len
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel ramfs_ok_text]
    mov edx, ramfs_ok_text_len
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
session_ok_text:
    db "storage-write ok", 10
session_ok_text_len equ $ - session_ok_text
config_dir:
    db "/tmp/config", 0
config_file:
    db "/tmp/config/storage.conf", 0
config_text:
    db "ramfs userland ok", 10
config_text_len equ $ - config_text
mkdir_ok_text:
    db "storage-mkdir ok", 10
mkdir_ok_text_len equ $ - mkdir_ok_text
create_ok_text:
    db "storage-create ok", 10
create_ok_text_len equ $ - create_ok_text
read_ok_text:
    db "storage-read ok", 10
read_ok_text_len equ $ - read_ok_text
delete_ok_text:
    db "storage-delete ok", 10
delete_ok_text_len equ $ - delete_ok_text
ramfs_ok_text:
    db "storage-ramfs ok", 10
ramfs_ok_text_len equ $ - ramfs_ok_text
failed_text:
    db "storage-write failed", 10
failed_text_len equ $ - failed_text
