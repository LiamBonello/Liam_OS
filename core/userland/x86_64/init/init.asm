bits 64

%define LIAM_SYSCALL_EXIT 1
%define LIAM_SYSCALL_WRITE 2
%define LIAM_SYSCALL_OPEN 3
%define LIAM_SYSCALL_READ 4
%define LIAM_SYSCALL_CLOSE 5
%define LIAM_SYSCALL_STAT 6
%define LIAM_SYSCALL_GET_ARG 7
%define LIAM_SYSCALL_EXEC 8
%define LIAM_SYSCALL_GETPID 9
%define LIAM_SYSCALL_YIELD 10
%define LIAM_RET_ENOENT 0xfffffffffffffffe

%define LIAM_STDIN 0
%define LIAM_STDOUT 1
%define LIAM_ARG_SHELL_MODE 0
%define LIAM_MODE_BUFFER_LEN 16
%define LIAM_READ_BUFFER_LEN 64
%define LIAM_LINE_BUFFER_LEN 80
%define LIAM_DEC_BUFFER_LEN 24
%define LIAM_MODE_BUFFER_OFFSET 0
%define LIAM_READ_BUFFER_OFFSET LIAM_MODE_BUFFER_LEN
%define LIAM_LINE_BUFFER_OFFSET (LIAM_MODE_BUFFER_LEN + LIAM_READ_BUFFER_LEN)
%define LIAM_STAT_SIZE_OFFSET (LIAM_MODE_BUFFER_LEN + LIAM_READ_BUFFER_LEN + LIAM_LINE_BUFFER_LEN)
%define LIAM_DEC_BUFFER_OFFSET (LIAM_STAT_SIZE_OFFSET + 8)
%define LIAM_STACK_BYTES 320

section .text
global _start

_start:
    sub rsp, LIAM_STACK_BYTES
    xor r15d, r15d

    mov eax, LIAM_SYSCALL_GETPID
    syscall

    mov eax, LIAM_SYSCALL_YIELD
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
    xor ebx, ebx

scan_input:
    cmp rbx, r13
    jae shell_loop

    mov al, [rsp + LIAM_READ_BUFFER_OFFSET + rbx]
    inc rbx

    cmp al, 13
    je carriage_return
    cmp al, 10
    je line_ready
    cmp al, 8
    je backspace
    cmp al, 127
    je backspace

    cmp r15, LIAM_LINE_BUFFER_LEN - 1
    jae scan_input
    mov [rsp + LIAM_LINE_BUFFER_OFFSET + r15], al

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_LINE_BUFFER_OFFSET + r15]
    mov edx, 1
    syscall

    inc r15
    jmp scan_input

backspace:
    test r15, r15
    jz scan_input
    dec r15

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel erase_text]
    mov edx, erase_text_len
    syscall
    jmp scan_input

carriage_return:
    cmp rbx, r13
    jae line_ready
    cmp byte [rsp + LIAM_READ_BUFFER_OFFSET + rbx], 10
    jne line_ready
    inc rbx

line_ready:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel newline_text]
    mov edx, newline_text_len
    syscall

    lea r12, [rsp + LIAM_LINE_BUFFER_OFFSET]

trim_trailing_spaces:
    test r15, r15
    jz line_terminated
    cmp byte [r12 + r15 - 1], ' '
    jne trim_leading_spaces
    dec r15
    jmp trim_trailing_spaces

trim_leading_spaces:
    test r15, r15
    jz line_terminated
    cmp byte [r12], ' '
    jne line_terminated
    dec r15
    xor r11d, r11d

shift_line_left:
    cmp r11, r15
    jae trim_leading_spaces
    mov al, [r12 + r11 + 1]
    mov [r12 + r11], al
    inc r11
    jmp shift_line_left

line_terminated:
    mov byte [r12 + r15], 0
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
    jb check_exec
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'e'
    jne check_exec
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'c'
    jne check_exec
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'h'
    jne check_exec
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'o'
    jne check_exec
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], ' '
    je command_echo

check_exec:
    cmp r15, 6
    jb check_stat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'e'
    jne check_stat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'x'
    jne check_stat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'e'
    jne check_stat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'c'
    jne check_stat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], ' '
    je command_exec

check_stat:
    cmp r15, 6
    jb check_cat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 's'
    jne check_cat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 't'
    jne check_cat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'a'
    jne check_cat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 't'
    jne check_cat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], ' '
    je command_stat

check_cat:
    cmp r15, 5
    jb command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'c'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'a'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 't'
    jne command_unknown
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], ' '
    je command_cat
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

command_exec:
    mov eax, LIAM_SYSCALL_EXEC
    lea rdi, [rsp + LIAM_LINE_BUFFER_OFFSET + 5]
    syscall
    test rax, rax
    jns command_done
    cmp rax, LIAM_RET_ENOENT
    je exec_not_found

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel exec_error_text]
    mov edx, exec_error_text_len
    syscall
    jmp command_done

exec_not_found:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel exec_not_found_text]
    mov edx, exec_not_found_text_len
    syscall
    jmp command_done

command_stat:
    mov eax, LIAM_SYSCALL_STAT
    lea rdi, [rsp + LIAM_LINE_BUFFER_OFFSET + 5]
    lea rsi, [rsp + LIAM_STAT_SIZE_OFFSET]
    syscall
    test rax, rax
    jnz stat_failed

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel stat_size_text]
    mov edx, stat_size_text_len
    syscall

    mov rax, [rsp + LIAM_STAT_SIZE_OFFSET]
    lea rdi, [rsp + LIAM_DEC_BUFFER_OFFSET + LIAM_DEC_BUFFER_LEN]
    xor ecx, ecx
    cmp rax, 0
    jne stat_convert_loop
    dec rdi
    mov byte [rdi], '0'
    mov ecx, 1
    jmp stat_write_number

stat_convert_loop:
    xor edx, edx
    mov r8, 10
    div r8
    add dl, '0'
    dec rdi
    mov [rdi], dl
    inc ecx
    test rax, rax
    jne stat_convert_loop

stat_write_number:
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
    jmp command_done

stat_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel stat_error_text]
    mov edx, stat_error_text_len
    syscall
    jmp command_done

command_cat:
    mov eax, LIAM_SYSCALL_OPEN
    lea rdi, [rsp + LIAM_LINE_BUFFER_OFFSET + 4]
    xor esi, esi
    syscall
    cmp rax, 3
    jb cat_failed
    cmp rax, 6
    ja cat_failed
    mov r14, rax

cat_read_loop:
    mov eax, LIAM_SYSCALL_READ
    mov rdi, r14
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    mov edx, LIAM_READ_BUFFER_LEN
    syscall
    test rax, rax
    jz cat_close
    cmp rax, LIAM_READ_BUFFER_LEN
    ja cat_close_failed

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    syscall
    jmp cat_read_loop

cat_close:
    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r14
    syscall
    jmp command_done_fresh

cat_close_failed:
    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r14
    syscall

cat_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel cat_error_text]
    mov edx, cat_error_text_len
    syscall
    jmp command_done_fresh

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

command_done_fresh:
    xor r15d, r15d
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel shell_prompt]
    mov edx, shell_prompt_len
    syscall
    jmp shell_loop

section .rodata
shell_banner:
    db "Liam_OS x86_64 shell online", 10
shell_banner_len equ $ - shell_banner
shell_prompt:
    db "$ "
shell_prompt_len equ $ - shell_prompt
help_text:
    db "commands: help, about, version, pid, echo, cat, stat, exec, clear, exit", 10
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
erase_text:
    db 8, " ", 8
erase_text_len equ $ - erase_text
newline_text:
    db 10
newline_text_len equ $ - newline_text
unknown_text:
    db "unknown command", 10
unknown_text_len equ $ - unknown_text
cat_error_text:
    db "cat: not found", 10
cat_error_text_len equ $ - cat_error_text
stat_size_text:
    db "size: "
stat_size_text_len equ $ - stat_size_text
stat_error_text:
    db "stat: not found", 10
stat_error_text_len equ $ - stat_error_text
exec_error_text:
    db "exec: not implemented", 10
exec_error_text_len equ $ - exec_error_text
exec_not_found_text:
    db "exec: not found", 10
exec_not_found_text_len equ $ - exec_not_found_text
bye_text:
    db "bye", 10
bye_text_len equ $ - bye_text
