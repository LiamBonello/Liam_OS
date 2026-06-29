bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_INPUT_STATUS 17
%define LIAM_SYSCALL_INPUT_EVENTS 18
%define LIAM_STDOUT 1
%define LIAM_STATUS_BUFFER_LEN 256
%define LIAM_EVENT_BUFFER_LEN 48
%define LIAM_DEC_BUFFER_LEN 24
%define LIAM_EVENT_BUFFER_OFFSET 0
%define LIAM_DEC_BUFFER_OFFSET LIAM_EVENT_BUFFER_LEN
%define LIAM_STATUS_BUFFER_OFFSET (LIAM_EVENT_BUFFER_LEN + LIAM_DEC_BUFFER_LEN)
%define LIAM_STACK_BYTES (LIAM_EVENT_BUFFER_LEN + LIAM_DEC_BUFFER_LEN + LIAM_STATUS_BUFFER_LEN)

section .text
global _start

_start:
    sub rsp, LIAM_STACK_BYTES

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel title_text]
    mov edx, title_text_len
    syscall

    mov eax, LIAM_SYSCALL_INPUT_STATUS
    lea rdi, [rsp + LIAM_STATUS_BUFFER_OFFSET]
    mov esi, LIAM_STATUS_BUFFER_LEN
    syscall
    test rax, rax
    jle .read_events

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_STATUS_BUFFER_OFFSET]
    syscall

.read_events:
    mov eax, LIAM_SYSCALL_INPUT_EVENTS
    lea rdi, [rsp + LIAM_EVENT_BUFFER_OFFSET]
    mov esi, 2
    syscall
    mov r12, rax

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel events_read_text]
    mov edx, events_read_text_len
    syscall

    mov rax, r12
    call write_rax_decimal_newline

    mov eax, LIAM_SYSCALL_EXIT
    xor edi, edi
    syscall

.halt:
    hlt
    jmp .halt

write_rax_decimal_newline:
    lea rdi, [rsp + 8 + LIAM_DEC_BUFFER_OFFSET + LIAM_DEC_BUFFER_LEN]
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
    db "Liam_OS input event service", 10
title_text_len equ $ - title_text
events_read_text:
    db "events read "
events_read_text_len equ $ - events_read_text
newline_text:
    db 10
newline_text_len equ $ - newline_text
