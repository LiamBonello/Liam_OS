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
%define LIAM_SYSCALL_PS 11
%define LIAM_SYSCALL_WAIT 12
%define LIAM_SYSCALL_DESKTOP_STATUS 13
%define LIAM_SYSCALL_WINDOW_PRESENT 14
%define LIAM_SYSCALL_TICKS 15
%define LIAM_SYSCALL_SLEEP_TICKS 16
%define LIAM_SYSCALL_INPUT_STATUS 17
%define LIAM_SYSCALL_INPUT_EVENTS 18
%define LIAM_SYSCALL_SPAWN 19
%define LIAM_RET_ENOENT 0xfffffffffffffffe

%define LIAM_STDIN 0
%define LIAM_STDOUT 1
%define LIAM_OPEN_WRITE 1
%define LIAM_OPEN_CREATE 2
%define LIAM_OPEN_TRUNCATE 4
%define LIAM_ARG_SHELL_MODE 0
%define LIAM_MODE_BUFFER_LEN 16
%define LIAM_READ_BUFFER_LEN 64
%define LIAM_LINE_BUFFER_LEN 80
%define LIAM_DEC_BUFFER_LEN 24
%define LIAM_PS_BUFFER_LEN 1024
%define LIAM_MODE_BUFFER_OFFSET 0
%define LIAM_READ_BUFFER_OFFSET LIAM_MODE_BUFFER_LEN
%define LIAM_LINE_BUFFER_OFFSET (LIAM_MODE_BUFFER_LEN + LIAM_READ_BUFFER_LEN)
%define LIAM_STAT_SIZE_OFFSET (LIAM_MODE_BUFFER_LEN + LIAM_READ_BUFFER_LEN + LIAM_LINE_BUFFER_LEN)
%define LIAM_DEC_BUFFER_OFFSET (LIAM_STAT_SIZE_OFFSET + 8)
%define LIAM_PS_BUFFER_OFFSET (LIAM_DEC_BUFFER_OFFSET + LIAM_DEC_BUFFER_LEN)
%define LIAM_STACK_BYTES 1248

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

    mov eax, LIAM_SYSCALL_GET_ARG
    mov edi, LIAM_ARG_SHELL_MODE
    lea rsi, [rsp + LIAM_MODE_BUFFER_OFFSET]
    mov edx, LIAM_MODE_BUFFER_LEN
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel shell_prompt]
    mov edx, shell_prompt_len
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

    mov eax, LIAM_SYSCALL_PS
    lea rdi, [rsp + LIAM_PS_BUFFER_OFFSET]
    mov esi, LIAM_PS_BUFFER_LEN
    syscall
    test rax, rax
    jle init_service_probe_spawn

    mov r11, rax
    lea rdi, [rsp + LIAM_PS_BUFFER_OFFSET]
    call ps_contains_windowd_service
    cmp eax, 1
    je init_services_done

init_service_probe_spawn:
    mov eax, LIAM_SYSCALL_SPAWN
    lea rdi, [rel windowd_service_exec_path]
    syscall
    test rax, rax
    jle init_windowd_service_failed

    mov r12, rax

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel init_service_pid_text]
    mov edx, init_service_pid_text_len
    syscall

    mov rax, r12
    call write_rax_decimal_newline
    jmp init_services_done

init_windowd_service_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel init_service_failed_text]
    mov edx, init_service_failed_text_len
    syscall

init_services_done:
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

check_direct_hello:
    cmp r15, 10
    jne check_direct_sysinfo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], '/'
    jne check_direct_sysinfo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'b'
    jne check_direct_sysinfo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'i'
    jne check_direct_sysinfo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'n'
    jne check_direct_sysinfo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], '/'
    jne check_direct_sysinfo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 5], 'h'
    jne check_direct_sysinfo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 6], 'e'
    jne check_direct_sysinfo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 7], 'l'
    jne check_direct_sysinfo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 8], 'l'
    jne check_direct_sysinfo
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 9], 'o'
    je command_direct_hello

check_direct_sysinfo:
    cmp r15, 12
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], '/'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'b'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'i'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'n'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], '/'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 5], 's'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 6], 'y'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 7], 's'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 8], 'i'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 9], 'n'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 10], 'f'
    jne check_help
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 11], 'o'
    je command_direct_sysinfo

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
    jne check_ps
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'p'
    jne check_ps
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'i'
    jne check_ps
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'd'
    je command_pid

check_ps:
    cmp r15, 2
    jne check_deskcheck
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'p'
    jne check_deskcheck
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 's'
    je command_ps

check_deskcheck:
    cmp r15, 9
    jne check_wait
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'd'
    jne check_wait
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'e'
    jne check_wait
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 's'
    jne check_wait
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'k'
    jne check_wait
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], 'c'
    jne check_wait
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 5], 'h'
    jne check_wait
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 6], 'e'
    jne check_wait
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 7], 'c'
    jne check_wait
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 8], 'k'
    je command_deskcheck

check_wait:
    cmp r15, 4
    jne check_service
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'w'
    jne check_service
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'a'
    jne check_service
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'i'
    jne check_service
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 't'
    je command_wait

check_service:
    cmp r15, 9
    jb check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 's'
    jne check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 'e'
    jne check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'r'
    jne check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 'v'
    jne check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], 'i'
    jne check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 5], 'c'
    jne check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 6], 'e'
    jne check_clear
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 7], ' '
    je command_service

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
    jb check_ls
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 's'
    jne check_ls
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 't'
    jne check_ls
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], 'a'
    jne check_ls
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 3], 't'
    jne check_ls
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 4], ' '
    je command_stat

check_ls:
    cmp r15, 3
    jb check_cat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], 'l'
    jne check_cat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 1], 's'
    jne check_cat
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 2], ' '
    je command_ls

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
    lea rdi, [rel help_path]
    call print_file
    jmp command_done_fresh

command_pid:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel pid_label]
    mov edx, pid_label_len
    syscall

    mov eax, LIAM_SYSCALL_GETPID
    syscall
    call write_rax_decimal_newline
    jmp command_done

command_ps:
    mov eax, LIAM_SYSCALL_PS
    lea rdi, [rsp + LIAM_PS_BUFFER_OFFSET]
    mov esi, LIAM_PS_BUFFER_LEN
    syscall
    test rax, rax
    jz ps_failed
    js ps_failed

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_PS_BUFFER_OFFSET]
    syscall
    jmp command_done_fresh

ps_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel ps_error_text]
    mov edx, ps_error_text_len
    syscall
    jmp command_done

command_deskcheck:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel deskcheck_title]
    mov edx, deskcheck_title_len
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel window_title_text]
    mov edx, window_title_text_len
    syscall

    mov eax, LIAM_SYSCALL_DESKTOP_STATUS
    lea rdi, [rsp + LIAM_PS_BUFFER_OFFSET]
    mov esi, LIAM_PS_BUFFER_LEN
    syscall
    test rax, rax
    jle deskcheck_window_present

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_PS_BUFFER_OFFSET]
    syscall

deskcheck_window_present:
    mov eax, LIAM_SYSCALL_WINDOW_PRESENT
    syscall
    cmp rax, 1
    jne deskcheck_window_failed

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel window_present_ok_text]
    mov edx, window_present_ok_text_len
    syscall
    jmp deskcheck_timer

deskcheck_window_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel window_present_failed_text]
    mov edx, window_present_failed_text_len
    syscall

deskcheck_timer:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel timer_title_text]
    mov edx, timer_title_text_len
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
    lea rsi, [rel ticks_elapsed_text]
    mov edx, ticks_elapsed_text_len
    syscall

    call write_rax_decimal_newline

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel sleep_ok_text]
    mov edx, sleep_ok_text_len
    syscall

deskcheck_input:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel input_title_text]
    mov edx, input_title_text_len
    syscall

    mov eax, LIAM_SYSCALL_INPUT_STATUS
    lea rdi, [rsp + LIAM_PS_BUFFER_OFFSET]
    mov esi, LIAM_PS_BUFFER_LEN
    syscall
    test rax, rax
    jle deskcheck_input_failed

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_PS_BUFFER_OFFSET]
    syscall
    jmp deskcheck_input_events

deskcheck_input_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel input_failed_text]
    mov edx, input_failed_text_len
    syscall

deskcheck_input_events:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel input_events_title_text]
    mov edx, input_events_title_text_len
    syscall

    mov eax, LIAM_SYSCALL_INPUT_EVENTS
    lea rdi, [rsp + LIAM_READ_BUFFER_OFFSET]
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

deskcheck_storage:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel storage_title_text]
    mov edx, storage_title_text_len
    syscall

    mov eax, LIAM_SYSCALL_OPEN
    lea rdi, [rel session_path]
    mov esi, LIAM_OPEN_WRITE | LIAM_OPEN_CREATE | LIAM_OPEN_TRUNCATE
    syscall
    cmp rax, 3
    jb deskcheck_storage_failed
    cmp rax, 6
    ja deskcheck_storage_failed
    mov r14, rax

    mov eax, LIAM_SYSCALL_WRITE
    mov rdi, r14
    lea rsi, [rel session_text]
    mov edx, session_text_len
    syscall
    cmp rax, session_text_len
    jne deskcheck_storage_close_failed

    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r14
    syscall

    mov eax, LIAM_SYSCALL_OPEN
    lea rdi, [rel session_path]
    xor esi, esi
    syscall
    cmp rax, 3
    jb deskcheck_storage_failed
    cmp rax, 6
    ja deskcheck_storage_failed
    mov r14, rax

    mov eax, LIAM_SYSCALL_READ
    mov rdi, r14
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    mov edx, LIAM_READ_BUFFER_LEN
    syscall
    test rax, rax
    jle deskcheck_storage_close_failed
    mov r13, rax

    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r14
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    mov rdx, r13
    syscall

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel storage_ok_text]
    mov edx, storage_ok_text_len
    syscall
    jmp command_done_fresh

deskcheck_storage_close_failed:
    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r14
    syscall

deskcheck_storage_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel storage_failed_text]
    mov edx, storage_failed_text_len
    syscall
    jmp command_done_fresh

command_wait:
    mov eax, LIAM_SYSCALL_WAIT
    lea rdi, [rsp + LIAM_STAT_SIZE_OFFSET]
    lea rsi, [rsp + LIAM_STAT_SIZE_OFFSET + 4]
    syscall
    test rax, rax
    jz wait_success
    cmp rax, LIAM_RET_ENOENT
    je wait_no_child

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel wait_error_text]
    mov edx, wait_error_text_len
    syscall
    jmp command_done

wait_no_child:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel wait_no_child_text]
    mov edx, wait_no_child_text_len
    syscall
    jmp command_done

wait_success:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel wait_pid_text]
    mov edx, wait_pid_text_len
    syscall

    mov eax, [rsp + LIAM_STAT_SIZE_OFFSET]
    call write_rax_decimal_newline

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel wait_exit_text]
    mov edx, wait_exit_text_len
    syscall

    mov eax, [rsp + LIAM_STAT_SIZE_OFFSET + 4]
    call write_rax_decimal_newline
    jmp command_done

command_clear:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel clear_text]
    mov edx, clear_text_len
    syscall
    jmp command_done

command_about:
    lea rdi, [rel about_path]
    call print_file
    jmp command_done_fresh

command_version:
    lea rdi, [rel version_path]
    call print_file
    jmp command_done_fresh

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

command_direct_hello:
    lea rdi, [rel hello_exec_path]
    jmp exec_path_pointer

command_direct_sysinfo:
    lea rdi, [rel sysinfo_exec_path]
    jmp exec_path_pointer

command_service:
    lea rdi, [rsp + LIAM_LINE_BUFFER_OFFSET + 8]
    mov eax, LIAM_SYSCALL_SPAWN
    syscall
    test rax, rax
    jle service_failed

    mov r12, rax

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel service_pid_text]
    mov edx, service_pid_text_len
    syscall

    mov rax, r12
    call write_rax_decimal_newline
    jmp command_done

service_failed:
    cmp rax, LIAM_RET_ENOENT
    je service_not_found

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel service_error_text]
    mov edx, service_error_text_len
    syscall
    jmp command_done

service_not_found:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel service_not_found_text]
    mov edx, service_not_found_text_len
    syscall
    jmp command_done

command_exec:
    lea rdi, [rsp + LIAM_LINE_BUFFER_OFFSET + 5]
    jmp exec_path_pointer

exec_path_pointer:
    mov eax, LIAM_SYSCALL_EXEC
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
    call write_rax_decimal_newline
    jmp command_done

stat_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel stat_error_text]
    mov edx, stat_error_text_len
    syscall
    jmp command_done

command_ls:
    mov eax, LIAM_SYSCALL_OPEN
    lea rdi, [rsp + LIAM_LINE_BUFFER_OFFSET + 3]
    xor esi, esi
    syscall
    cmp rax, 3
    jb ls_failed
    cmp rax, 6
    ja ls_failed
    mov r14, rax

ls_read_loop:
    mov eax, LIAM_SYSCALL_READ
    mov rdi, r14
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    mov edx, LIAM_READ_BUFFER_LEN
    syscall
    test rax, rax
    jz ls_close
    cmp rax, LIAM_READ_BUFFER_LEN
    ja ls_close_failed

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    syscall
    jmp ls_read_loop

ls_close:
    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r14
    syscall
    jmp command_done_fresh

ls_close_failed:
    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r14
    syscall

ls_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel ls_error_text]
    mov edx, ls_error_text_len
    syscall
    jmp command_done_fresh

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
    cmp byte [rsp + LIAM_LINE_BUFFER_OFFSET + 0], '/'
    je command_direct_exec

    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel unknown_text]
    mov edx, unknown_text_len
    syscall
    jmp command_done

command_direct_exec:
    lea rdi, [rsp + LIAM_LINE_BUFFER_OFFSET]
    jmp exec_path_pointer

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

ps_contains_windowd_service:
    xor r8d, r8d

ps_contains_windowd_service_next:
    cmp r8, r11
    jae ps_contains_windowd_service_not_found

    mov al, [rdi + r8]
    cmp al, '/'
    jne ps_contains_windowd_service_advance

    mov rax, r11
    sub rax, r8
    cmp rax, windowd_service_exec_path_len
    jb ps_contains_windowd_service_not_found

    mov r10, rdi
    add r10, r8
    lea rsi, [rel windowd_service_exec_path]
    xor r9d, r9d

ps_contains_windowd_service_compare:
    cmp r9, windowd_service_exec_path_len
    jae ps_contains_windowd_service_found

    mov al, [r10 + r9]
    cmp al, [rsi + r9]
    jne ps_contains_windowd_service_advance

    inc r9
    jmp ps_contains_windowd_service_compare

ps_contains_windowd_service_found:
    mov eax, 1
    ret

ps_contains_windowd_service_advance:
    inc r8
    jmp ps_contains_windowd_service_next

ps_contains_windowd_service_not_found:
    xor eax, eax
    ret

print_file:
    mov eax, LIAM_SYSCALL_OPEN
    xor esi, esi
    syscall
    cmp rax, 3
    jb print_file_failed
    cmp rax, 6
    ja print_file_failed
    mov r14, rax

print_file_read_loop:
    mov eax, LIAM_SYSCALL_READ
    mov rdi, r14
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    mov edx, LIAM_READ_BUFFER_LEN
    syscall
    test rax, rax
    jz print_file_close
    cmp rax, LIAM_READ_BUFFER_LEN
    ja print_file_close_failed

    mov rdx, rax
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rsp + LIAM_READ_BUFFER_OFFSET]
    syscall
    jmp print_file_read_loop

print_file_close:
    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r14
    syscall
    ret

print_file_close_failed:
    mov eax, LIAM_SYSCALL_CLOSE
    mov rdi, r14
    syscall

print_file_failed:
    mov eax, LIAM_SYSCALL_WRITE
    mov edi, LIAM_STDOUT
    lea rsi, [rel file_error_text]
    mov edx, file_error_text_len
    syscall
    ret

write_rax_decimal_newline:
    lea rdi, [rsp + 8 + LIAM_DEC_BUFFER_OFFSET + LIAM_DEC_BUFFER_LEN]
    xor ecx, ecx
    cmp rax, 0
    jne decimal_convert_loop
    dec rdi
    mov byte [rdi], '0'
    mov ecx, 1
    jmp decimal_write_number

decimal_convert_loop:
    xor edx, edx
    mov r8, 10
    div r8
    add dl, '0'
    dec rdi
    mov [rdi], dl
    inc ecx
    test rax, rax
    jne decimal_convert_loop

decimal_write_number:
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
shell_banner:
    db "Liam_OS x86_64 shell online", 10
shell_banner_len equ $ - shell_banner
shell_prompt:
    db "$ "
shell_prompt_len equ $ - shell_prompt
help_path:
    db "/usr/share/help.txt", 0
about_path:
    db "/etc/motd", 0
version_path:
    db "/proc/version", 0
hello_exec_path:
    db "/bin/hello", 0
sysinfo_exec_path:
    db "/bin/sysinfo", 0
windowd_service_exec_path:
    db "/bin/windowd-service", 0
windowd_service_exec_path_len equ $ - windowd_service_exec_path - 1
session_path:
    db "/tmp/session.txt", 0
session_text:
    db "session storage ok", 10
session_text_len equ $ - session_text
pid_label:
    db "pid: "
pid_label_len equ $ - pid_label
deskcheck_title:
    db "Liam_OS desktop readiness check", 10
deskcheck_title_len equ $ - deskcheck_title
window_title_text:
    db "Liam_OS window service", 10
window_title_text_len equ $ - window_title_text
window_present_ok_text:
    db "window-present ok", 10
window_present_ok_text_len equ $ - window_present_ok_text
window_present_failed_text:
    db "window-present failed", 10
window_present_failed_text_len equ $ - window_present_failed_text
timer_title_text:
    db "Liam_OS timer service", 10
timer_title_text_len equ $ - timer_title_text
ticks_elapsed_text:
    db "ticks elapsed "
ticks_elapsed_text_len equ $ - ticks_elapsed_text
sleep_ok_text:
    db "sleep-ticks ok", 10
sleep_ok_text_len equ $ - sleep_ok_text
input_title_text:
    db "Liam_OS input service", 10
input_title_text_len equ $ - input_title_text
input_failed_text:
    db "input-status failed", 10
input_failed_text_len equ $ - input_failed_text
input_events_title_text:
    db "Liam_OS input event service", 10
input_events_title_text_len equ $ - input_events_title_text
events_read_text:
    db "events read "
events_read_text_len equ $ - events_read_text
storage_title_text:
    db "Liam_OS storage service", 10
storage_title_text_len equ $ - storage_title_text
storage_ok_text:
    db "storage-write ok", 10
storage_ok_text_len equ $ - storage_ok_text
storage_failed_text:
    db "storage-write failed", 10
storage_failed_text_len equ $ - storage_failed_text
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
ls_error_text:
    db "ls: not found", 10
ls_error_text_len equ $ - ls_error_text
cat_error_text:
    db "cat: not found", 10
cat_error_text_len equ $ - cat_error_text
stat_size_text:
    db "size: "
stat_size_text_len equ $ - stat_size_text
stat_error_text:
    db "stat: not found", 10
stat_error_text_len equ $ - stat_error_text
ps_error_text:
    db "ps: unavailable", 10
ps_error_text_len equ $ - ps_error_text
wait_pid_text:
    db "wait: pid "
wait_pid_text_len equ $ - wait_pid_text
wait_exit_text:
    db "wait: exit "
wait_exit_text_len equ $ - wait_exit_text
wait_no_child_text:
    db "wait: no child", 10
wait_no_child_text_len equ $ - wait_no_child_text
wait_error_text:
    db "wait: failed", 10
wait_error_text_len equ $ - wait_error_text

init_service_pid_text:
    db "init: service pid "
init_service_pid_text_len equ $ - init_service_pid_text

init_service_failed_text:
    db "init: service failed", 10
init_service_failed_text_len equ $ - init_service_failed_text

service_pid_text:
    db "service: pid "
service_pid_text_len equ $ - service_pid_text

service_error_text:
    db "service: failed", 10
service_error_text_len equ $ - service_error_text

service_not_found_text:
    db "service: not found", 10
service_not_found_text_len equ $ - service_not_found_text

exec_error_text:
    db "exec: not implemented", 10
exec_error_text_len equ $ - exec_error_text
exec_not_found_text:
    db "exec: not found", 10
exec_not_found_text_len equ $ - exec_not_found_text
file_error_text:
    db "file: not found", 10
file_error_text_len equ $ - file_error_text
bye_text:
    db "bye", 10
bye_text_len equ $ - bye_text