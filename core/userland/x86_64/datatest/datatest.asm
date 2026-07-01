bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_STDOUT 1

section .text
global _start

_start:
    mov eax, [rel data_counter]
    cmp eax, 41
    jne .failed

    inc dword [rel data_counter]

    mov eax, [rel data_counter]
    cmp eax, 42
    jne .failed

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel ok_text]
    mov edx, ok_text_len
    syscall

    mov eax, LIAM_SYSCALL_EXIT
    xor edi, edi
    syscall

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
ok_text:
    db "selfcheck: writable data segment ok", 10
ok_text_len equ $ - ok_text
failed_text:
    db "selfcheck: writable data segment failed", 10
failed_text_len equ $ - failed_text

section .data
data_counter:
    dd 41
