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

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel shell_prompt]
    mov edx, shell_prompt_len
    syscall

    mov eax, LIAM_SYSCALL_GET_ARG
    mov edi, LIAM_ARG_SHELL_MODE
    lea rsi, [rsp + LIAM_MODE_BUFFER_OFFSET]
    mov edx, LIAM_MODE_BUFFER_LEN
    syscall

shell_loop:
    mov eax, LIAM_SYSCALL_READ
    mov edi, LIAM_STDIN
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    mov edx, LIAM_READ_BUFFER_LEN
    syscall

    test rax, rax
    jg process_input

    mov eax, LIAM_SYSCALL_YIELD
    syscall
    jmp shell_loop

process_input:
    mov r13, rax

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    mov rdx, r13
    syscall

    xor ebx, ebx

scan_input:
    cmp rbx, r13
    jae shell_loop

    mov al, [rsp + LIAM_READ_BUFFER_OFFSET + rbx]
    inc rbx

    cmp al, 13
    je line_ready
    cmp al, 10
    je line_ready
    cmp al, 8
    je backspace
    cmp al, 127
    je backspace

    cmp r15, LIAM_LINE_BUFFER_LEN - 1
    jae scan_input
    mov [rsp + LIAM_LINE_BUFFER_OFFSET + r15], al
    inc r15
    jmp scan_input

backspace:
    test r15, r15
    jz scan_input
    dec r15
    jmp scan_input

line_ready:
    mov byte [rsp + LIAM_LINE_BUFFER_OFFSET + r15], 0
    jmp handle_line

exit_success:
    mov eax, LIAM_SYSCALL_EXIT
    xor edi, edi
    syscall
    jmp halt

halt:
    hlt
    jmp halt

handle_line:
    cmp r15, 0
    je command_done

check_help:
    cmp r15, 4
    jne check_pid
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'h'
    jne check_exit
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'e'
    jne check_exit
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'l'
    jne check_exit
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'p'
    je command_help

check_exit:
    cmp r15, 4
    jne check_pid
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'e'
    jne check_pid
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'x'
    jne check_pid
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'i'
    jne check_pid
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 't'
    je command_exit

check_pid:
    cmp r15, 3
    jne check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'p'
    jne check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'i'
    jne check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'd'
    je command_pid

check_clear:
    cmp r15, 5
    jne check_about
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'c'
    jne check_about
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'l'
    jne check_about
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'e'
    jne check_about
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'a'
    jne check_about
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], 'r'
    je command_clear

check_about:
    cmp r15, 5
    jne check_version
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'a'
    jne check_version
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'b'
    jne check_version
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'o'
    jne check_version
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'u'
    jne check_version
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], 't'
    je command_about

check_version:
    cmp r15, 7
    jne check_echo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'v'
    jne check_echo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'e'
    jne check_echo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'r'
    jne check_echo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 's'
    jne check_echo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], 'i'
    jne check_echo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 5], 'o'
    jne check_echo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 6], 'n'
    je command_version

check_echo:
    cmp r15, 5
    jb command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'e'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'c'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'h'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'o'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], ' '
    je command_echo
    jmp command_unknown

command_help:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel help_text]
    mov edx, help_text_len
    syscall
    jmp command_done

command_pid:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel pid_text]
    mov edx, pid_text_len
    syscall
    jmp command_done

command_clear:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel clear_text]
    mov edx, clear_text_len
    syscall
    jmp command_done

command_about:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel about_text]
    mov edx, about_text_len
    syscall
    jmp command_done

command_version:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel version_text]
    mov edx, version_text_len
    syscall
    jmp command_done

command_echo:
    mov rdx, r15
    sub rdx, 5
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_LINE_BUFFER_OFFSET + 5]
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel newline_text]
    mov edx, newline_text_len
    syscall
    jmp command_done

command_exit:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel bye_text]
    mov edx, bye_text_len
    syscall
    jmp exit_success

command_unknown:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel unknown_text]
    mov edx, unknown_text_len
    syscall
    jmp command_done

command_done:
    xor r15d, r15d
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel shell_prompt]
    mov edx, shell_prompt_len
    syscall
    jmp scan_input

section .rodata
shell_banner:
    db "Liam_OS x86_64 shell online", 10
shell_banner_len equ $ - shell_banner
shell_prompt:
    db "$ "
shell_prompt_len equ $ - shell_prompt
help_text:
    db "commands: help, about, version, pid, echo, clear, exit", 10
help_text_len equ $ - help_text
pid_text:
    db "pid: 1", 10
pid_text_len equ $ - pid_text
about_text:
    db "Liam_OS Core x86_64 user shell", 10
about_text_len equ $ - about_text
version_text:
    db "Liam_OS Core x86_64 dev", 10
version_text_len equ $ - version_text
clear_text:
    db 27, "[2J", 27, "[H"
clear_text_len equ $ - clear_text
newline_text:
    db 10
newline_text_len equ $ - newline_text
unknown_text:
    db "unknown command", 10
unknown_text_len equ $ - unknown_text
bye_text:
    db "bye", 10
bye_text_len equ $ - bye_text
