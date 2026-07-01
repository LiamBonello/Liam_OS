bits 64

%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_YIELD 10
%define LIAM_SYSCALL_DESKTOP_STATUS 13
%define LIAM_SYSCALL_WINDOW_PRESENT 14
%define LIAM_SYSCALL_TICKS 15
%define LIAM_SYSCALL_SLEEP_TICKS 16
%define LIAM_SYSCALL_INPUT_STATUS 17
%define LIAM_SYSCALL_INPUT_EVENTS 18
%define LIAM_STDOUT 1
%define LIAM_STATUS_BUFFER_LEN 1024
%define LIAM_EVENT_BUFFER_LEN 48
%define LIAM_STACK_BYTES (LIAM_STATUS_BUFFER_LEN + LIAM_EVENT_BUFFER_LEN)
%define LIAM_EVENT_BUFFER_OFFSET 0
%define LIAM_STATUS_BUFFER_OFFSET LIAM_EVENT_BUFFER_LEN
%define LIAM_SESSION_FRAME_TICKS 2

section .text
global _start

_start:
    sub rsp, LIAM_STACK_BYTES

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel title_text]
    mov edx, title_text_len
    syscall

    mov eax, LIAM_SYSCALL_DESKTOP_STATUS
    lea rdi, [rsp + LIAM_STATUS_BUFFER_OFFSET]
    mov esi, LIAM_STATUS_BUFFER_LEN
    syscall
    test rax, rax
    jle .desktop_status_failed

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_STATUS_BUFFER_OFFSET]
    syscall
    jmp .present_window

.desktop_status_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel status_failed_text]
    mov edx, status_failed_text_len
    syscall

.present_window:
    mov eax, LIAM_SYSCALL_WINDOW_PRESENT
    syscall
    cmp rax, 1
    jne .present_failed

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel present_ok_text]
    mov edx, present_ok_text_len
    syscall
    jmp .ready

.present_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel present_failed_text]
    mov edx, present_failed_text_len
    syscall

.ready:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel ready_text]
    mov edx, ready_text_len
    syscall

.session_loop:
    mov eax, LIAM_SYSCALL_INPUT_STATUS
    lea rdi, [rsp + LIAM_STATUS_BUFFER_OFFSET]
    mov esi, LIAM_STATUS_BUFFER_LEN
    syscall
    test rax, rax
    jle .present_frame

    mov eax, LIAM_SYSCALL_INPUT_EVENTS
    lea rdi, [rsp + LIAM_EVENT_BUFFER_OFFSET]
    mov esi, 2
    syscall

.present_frame:
    mov eax, LIAM_SYSCALL_WINDOW_PRESENT
    syscall

    mov eax, LIAM_SYSCALL_SLEEP_TICKS
    mov edi, LIAM_SESSION_FRAME_TICKS
    syscall

    mov eax, LIAM_SYSCALL_YIELD
    syscall

    jmp .session_loop

section .rodata
title_text:
    db "Liam_OS desktop session", 10
title_text_len equ $ - title_text
status_failed_text:
    db "sessiond: desktop status unavailable", 10
status_failed_text_len equ $ - status_failed_text
present_ok_text:
    db "sessiond: system window presented", 10
present_ok_text_len equ $ - present_ok_text
present_failed_text:
    db "sessiond: system window present failed", 10
present_failed_text_len equ $ - present_failed_text
ready_text:
    db "sessiond: ready", 10
ready_text_len equ $ - ready_text
