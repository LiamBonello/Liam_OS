bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_TICKS 15
%define LIAM_SYSCALL_SLEEP_TICKS 16
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

    mov eax, LIAM_SYSCALL_TICKS
    syscall
    mov r12, rax

    mov eax, LIAM_SYSCALL_SLEEP_TICKS
    mov edi, 2
    syscall

    mov eax, LIAM_SYSCALL_TICKS
    syscall
    sub rax, r12

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel delta_text]
    mov edx, delta_text_len
    syscall

    call write_rax_decimal_newline

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel sleep_ok_text]
    mov edx, sleep_ok_text_len
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
    db "Liam_OS timer service", 10
title_text_len equ $ - title_text
delta_text:
    db "ticks elapsed "
delta_text_len equ $ - delta_text
sleep_ok_text:
    db "sleep-ticks ok", 10
sleep_ok_text_len equ $ - sleep_ok_text
newline_text:
    db 10
newline_text_len equ $ - newline_text
