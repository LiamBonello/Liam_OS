bits 32

_start:
    call .get_eip

.get_eip:
    pop esi

    mov ebx, esi
    add ebx, .banner - .get_eip
    mov ecx, .banner_len
    call .write_buffer

    mov ebx, esi
    add ebx, .write_label - .get_eip
    mov ecx, .write_label_len
    call .write_buffer

    mov ebx, 0xC0000000
    mov ecx, 1
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80
    test eax, eax
    js .write_ok
    jmp .failed

.write_ok:
    call .write_ok_message

    mov ebx, esi
    add ebx, .open_label - .get_eip
    mov ecx, .open_label_len
    call .write_buffer

    mov ebx, 0xC0000000
    mov ecx, 0
    mov edx, 0
    mov eax, 6              ; LIAM_SYSCALL_OPEN
    int 0x80
    test eax, eax
    js .open_ok
    jmp .failed

.open_ok:
    call .write_ok_message

    mov ebx, esi
    add ebx, .stat_path_label - .get_eip
    mov ecx, .stat_path_label_len
    call .write_buffer

    mov ebx, 0xC0000000
    mov ecx, esi
    add ecx, .stat_buffer - .get_eip
    mov edx, 0
    mov eax, 9              ; LIAM_SYSCALL_STAT
    int 0x80
    test eax, eax
    js .stat_path_ok
    jmp .failed

.stat_path_ok:
    call .write_ok_message

    mov ebx, esi
    add ebx, .stat_out_label - .get_eip
    mov ecx, .stat_out_label_len
    call .write_buffer

    mov ebx, esi
    add ebx, .valid_path - .get_eip
    mov ecx, 0xC0000000
    mov edx, 0
    mov eax, 9              ; LIAM_SYSCALL_STAT
    int 0x80
    test eax, eax
    js .stat_out_ok
    jmp .failed

.stat_out_ok:
    call .write_ok_message

    mov ebx, esi
    add ebx, .get_arg_label - .get_eip
    mov ecx, .get_arg_label_len
    call .write_buffer

    mov ebx, 0
    mov ecx, 0xC0000000
    mov edx, 16
    mov eax, 11             ; LIAM_SYSCALL_GET_ARG
    int 0x80
    test eax, eax
    js .get_arg_ok
    jmp .failed

.get_arg_ok:
    call .write_ok_message

    mov ebx, esi
    add ebx, .exec_label - .get_eip
    mov ecx, .exec_label_len
    call .write_buffer

    mov ebx, 0xC0000000
    mov ecx, esi
    add ecx, .empty_args - .get_eip
    mov edx, 0
    mov eax, 12             ; LIAM_SYSCALL_EXEC
    int 0x80
    test eax, eax
    js .exec_ok
    jmp .failed

.exec_ok:
    call .write_ok_message

    mov ebx, esi
    add ebx, .read_label - .get_eip
    mov ecx, .read_label_len
    call .write_buffer

    mov ebx, esi
    add ebx, .valid_path - .get_eip
    mov ecx, 0
    mov edx, 0
    mov eax, 6              ; LIAM_SYSCALL_OPEN
    int 0x80
    test eax, eax
    js .failed

    mov edi, eax

    mov ebx, edi
    mov ecx, 0xC0000000
    mov edx, 16
    mov eax, 7              ; LIAM_SYSCALL_READ
    int 0x80
    test eax, eax
    js .read_ok
    jmp .failed

.read_ok:
    call .write_ok_message

    mov ebx, edi
    mov ecx, 0
    mov edx, 0
    mov eax, 8              ; LIAM_SYSCALL_CLOSE
    int 0x80
    test eax, eax
    js .failed

    mov ebx, esi
    add ebx, .passed_message - .get_eip
    mov ecx, .passed_message_len
    call .write_buffer

    mov ebx, 0
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

.failed:
    mov ebx, esi
    add ebx, .failed_message - .get_eip
    mov ecx, .failed_message_len
    call .write_buffer

    mov ebx, 1
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

    ud2

.write_ok_message:
    mov ebx, esi
    add ebx, .ok_message - .get_eip
    mov ecx, .ok_message_len

.write_buffer:
    push eax
    push edx

    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

    pop edx
    pop eax
    ret

.banner:
    db 'sysbadptr: syscall bad pointer regression', 10
.banner_len equ $ - .banner

.write_label:
    db 'sysbadptr: write bad buffer: '
.write_label_len equ $ - .write_label

.open_label:
    db 'sysbadptr: open bad path: '
.open_label_len equ $ - .open_label

.stat_path_label:
    db 'sysbadptr: stat bad path: '
.stat_path_label_len equ $ - .stat_path_label

.stat_out_label:
    db 'sysbadptr: stat bad output: '
.stat_out_label_len equ $ - .stat_out_label

.get_arg_label:
    db 'sysbadptr: get_arg bad output: '
.get_arg_label_len equ $ - .get_arg_label

.exec_label:
    db 'sysbadptr: exec bad path: '
.exec_label_len equ $ - .exec_label

.read_label:
    db 'sysbadptr: read bad output: '
.read_label_len equ $ - .read_label

.ok_message:
    db 'ok', 10
.ok_message_len equ $ - .ok_message

.passed_message:
    db 'sysbadptr: all bad pointer checks passed', 10
.passed_message_len equ $ - .passed_message

.failed_message:
    db 'sysbadptr: FAILED', 10
.failed_message_len equ $ - .failed_message

.valid_path:
    db '/etc/os-release', 0

.empty_args:
    db 0

.stat_buffer:
    times 20 db 0
