bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_READ 4
%define LIAM_SYSCALL_GET_ARG 7
%define LIAM_SYSCALL_GETPID 9
%define LIAM_SYSCALL_YIELD 10

%define LIAM_STDIN 0
%define LIAM_STDOUT 1
%define LIAM_ARG_SHELL_MODE 0
%define LIAM_MODE_BUFFER_LEN 16
%define LIAM_READ_BUFFER_LEN 64
%define LIAM_LINE_BUFFER_LEN 80
%define LIAM_IDLE_POLL_LIMIT 3
%define LIAM_MODE_BUFFER_OFFSET 0
%define LIAM_READ_BUFFER_OFFSET LIAM_MODE_BUFFER_LEN
%define LIAM_LINE_BUFFER_OFFSET (LIAM_MODE_BUFFER_LEN + LIAM_READ_BUFFER_LEN)
%define LIAM_STACK_BYTES 256

section .text
global _start

_start:
    sub rsp, LIAM_STACK_BYTES
    xor r15d, r15d

    mov eax, LIAM_SYSCALL_GETPID
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel shell_banner]
    mov edx, shell_banner_len
    syscall

    xor r14d, r14d
    mov eax, LIAM_SYSCALL_GET_ARG
    mov edi, LIAM_ARG_SHELL_MODE
    lea rsi, [rsp + LIAM_MODE_BUFFER_OFFSET]
    mov edx, LIAM_MODE_BUFFER_LEN
    syscall
    test rax, rax
    jle .mode_ready
    mov r14d, 1

.mode_ready:
    xor r12d, r12d

.shell_loop:
    mov eax, LIAM_SYSCALL_READ
    mov edi, LIAM_STDIN
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    mov edx, LIAM_READ_BUFFER_LEN
    syscall

    test rax, rax
    jg .process_input

    mov eax, LIAM_SYSCALL_YIELD
    syscall

    test r14d, r14d
    jnz .shell_loop

    inc r12d
    cmp r12d, LIAM_IDLE_POLL_LIMIT
    jb .shell_loop

    jmp exit_success

.process_input:
    xor r12d, r12d
    mov r13, rax

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    mov rdx, r13
    syscall

    xor ebx, ebx

.scan_input:
    cmp rbx, r13
    jae .shell_loop

    mov al, [rsp + LIAM_READ_BUFFER_OFFSET + rbx]
    inc rbx

    cmp al, 13
    je .line_ready
    cmp al, 10
    je .line_ready
    cmp al, 8
    je .backspace
    cmp al, 127
    je .backspace

    cmp r15, LIAM_LINE_BUFFER_LEN - 1
    jae .scan_input
    mov [rsp + LIAM_LINE_BUFFER_OFFSET + r15], al
    inc r15
    jmp .scan_input

.backspace:
    test r15, r15
    jz .scan_input
    dec r15
    jmp .scan_input

.line_ready:
    mov byte [rsp + LIAM_LINE_BUFFER_OFFSET + r15], 0
    call handle_line
    test rax, rax
    jnz exit_success
    jmp .scan_input

.halt:
    hlt
    jmp .halt

exit_success:
    mov eax, LIAM_SYSCALL_EXIT
    xor edi, edi
    syscall
    jmp _start.halt

handle_line:
    cmp r15, 0
    je command_empty

    cmp r15, 4
    jne .check_pid
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'h'
    jne .check_exit
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'e'
    jne .check_exit
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'l'
    jne .check_exit
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'p'
    je command_help

.check_exit:
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'e'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'x'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'i'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 't'
    je command_exit
    jmp command_unknown

.check_pid:
    cmp r15, 3
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'p'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'i'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'd'
    je command_pid
    jmp command_unknown

command_empty:
    xor r15d, r15d
    call write_prompt
    xor eax, eax
    ret

command_help:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel help_text]
    mov edx, help_text_len
    syscall
    xor r15d, r15d
    call write_prompt
    xor eax, eax
    ret

command_pid:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel pid_text]
    mov edx, pid_text_len
    syscall
    xor r15d, r15d
    call write_prompt
    xor eax, eax
    ret

command_exit:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel bye_text]
    mov edx, bye_text_len
    syscall
    mov eax, 1
    ret

command_unknown:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel unknown_text]
    mov edx, unknown_text_len
    syscall
    xor r15d, r15d
    call write_prompt
    xor eax, eax
    ret

write_prompt:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel shell_prompt]
    mov edx, shell_prompt_len
    syscall
    ret

section .rodata
shell_banner:
    db "Liam_OS x86_64 shell online", 10
shell_prompt:
    db "$ "
shell_prompt_len equ $ - shell_prompt
shell_banner_len equ $ - shell_banner
help_text:
    db "commands: help, pid, exit", 10
help_text_len equ $ - help_text
pid_text:
    db "pid: 1", 10
pid_text_len equ $ - pid_text
unknown_text:
    db "unknown command", 10
unknown_text_len equ $ - unknown_text
bye_text:
    db "bye", 10
bye_text_len equ $ - bye_text
