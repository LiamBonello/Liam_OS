bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_GETPID 9
%define LIAM_STDOUT 1
%define LIAM_DEC_BUFFER_LEN 24

section .text
global _start

_start:
    sub rsp, LIAM_DEC_BUFFER_LEN

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel title_text]
    mov edx, title_text_len
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel pid_label]
    mov edx, pid_label_len
    syscall

    mov eax, LIAM_SYSCALL_GETPID
    syscall
    call write_rax_decimal_newline

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

write_rax_decimal_newline:
    lea rdi, [rsp + 8 + LIAM_DEC_BUFFER_LEN]
    xor ecx, ecx
    cmp rax, 0
    jne .convert_loop
    dec rdi
    mov byte [rdi], '0'
    mov ecx, 1
    jmp .write_number

.convert_loop:
    xor edx, edx
    mov r8, 10
    div r8
    add dl, '0'
    dec rdi
    mov [rdi], dl
    inc ecx
    test rax, rax
    jne .convert_loop

.write_number:
    mov eax, LIAM_SYSCALL_WRITE
    mov rsi, rdi
    mov edi, LIAM_STDOUT
    mov edx, ecx
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel newline_text]
    mov edx, newline_text_len
    syscall
    ret

section .rodata
title_text:
    db "Liam_OS sysinfo", 10
    db "arch: x86_64", 10
    db "mode: ring3 user ELF", 10
title_text_len equ $ - title_text
pid_label:
    db "pid: "
pid_label_len equ $ - pid_label
features_text:
    db "features: syscall, vfs, exec, desktop-services", 10
features_text_len equ $ - features_text
newline_text:
    db 10
newline_text_len equ $ - newline_text
