bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_PS 11
%define LIAM_SYSCALL_TICKS 15
%define LIAM_STDOUT 1
%define LIAM_PS_BUFFER_LEN 1024
%define LIAM_DEC_BUFFER_LEN 24
%define LIAM_STACK_BYTES (LIAM_PS_BUFFER_LEN + LIAM_DEC_BUFFER_LEN)
%define LIAM_PS_BUFFER_OFFSET 0
%define LIAM_DEC_BUFFER_OFFSET LIAM_PS_BUFFER_LEN

section .text
global _start

_start:
    sub rsp, LIAM_STACK_BYTES

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel title_text]
    mov edx, title_text_len
    syscall

    mov eax, LIAM_SYSCALL_TICKS
    syscall
    mov r12, rax

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel ticks_text]
    mov edx, ticks_text_len
    syscall

    mov rax, r12
    call write_rax_decimal_newline

    mov eax, LIAM_SYSCALL_PS
    lea rdi, [rsp + LIAM_PS_BUFFER_OFFSET]
    mov esi, LIAM_PS_BUFFER_LEN
    syscall
    test rax, rax
    jle .ready

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_PS_BUFFER_OFFSET]
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

write_rax_decimal_newline:
    lea rdi, [rsp + LIAM_DEC_BUFFER_OFFSET + LIAM_DEC_BUFFER_LEN]
    xor ecx, ecx
    cmp rax, 0
    jne .decimal_convert_loop
    dec rdi
    mov byte [rdi], '0'
    mov ecx, 1
    jmp .decimal_write_number

.decimal_convert_loop:
    xor edx, edx
    mov r8, 10
    div r8
    add dl, '0'
    dec rdi
    mov [rdi], dl
    inc ecx
    test rax, rax
    jne .decimal_convert_loop

.decimal_write_number:
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
    db "Liam_OS System Monitor", 10
title_text_len equ $ - title_text
ticks_text:
    db "monitor: ticks "
ticks_text_len equ $ - ticks_text
ready_text:
    db "system-monitor: ready", 10
ready_text_len equ $ - ready_text
newline_text:
    db 10
newline_text_len equ $ - newline_text
