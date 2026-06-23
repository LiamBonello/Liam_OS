bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_READ 4
%define LIAM_SYSCALL_GETPID 9
%define LIAM_SYSCALL_YIELD 10

%define LIAM_STDIN 0
%define LIAM_STDOUT 1
%define LIAM_READ_BUFFER_LEN 64
%define LIAM_IDLE_POLL_LIMIT 3

section .text
global _start

_start:
    mov eax, LIAM_SYSCALL_GETPID
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel shell_banner]
    mov edx, shell_banner_len
    syscall

    xor r12d, r12d

.shell_loop:
    mov eax, LIAM_SYSCALL_READ
    mov edi, LIAM_STDIN
    lea rsi, [rsp - LIAM_READ_BUFFER_LEN]
    mov edx, LIAM_READ_BUFFER_LEN
    syscall

    test rax, rax
    jg .echo_input

    mov eax, LIAM_SYSCALL_YIELD
    syscall

    inc r12d
    cmp r12d, LIAM_IDLE_POLL_LIMIT
    jb .shell_loop

    mov eax, LIAM_SYSCALL_EXIT
    xor edi, edi
    syscall
    jmp .halt

.echo_input:
    xor r12d, r12d
    mov r13, rax

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp - LIAM_READ_BUFFER_LEN]
    mov rdx, r13
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel shell_prompt]
    mov edx, shell_prompt_len
    syscall

    jmp .shell_loop

.halt:
    hlt
    jmp .halt

section .rodata
shell_banner:
    db "Liam_OS x86_64 shell online", 10
shell_prompt:
    db "$ "
shell_prompt_len equ $ - shell_prompt
shell_banner_len equ $ - shell_banner
