bits 32

_start:
    call .get_eip

.get_eip:
    pop esi

    mov ebx, 0
    mov ecx, 0
    mov edx, 0
    mov eax, 10             ; LIAM_SYSCALL_GET_ARGC
    int 0x80

    cmp eax, 2
    jb .use_default_path

    mov ebx, 1              ; argv[1]
    mov ecx, esi
    add ecx, .path_buffer - .get_eip
    mov edx, .path_buffer_len
    mov eax, 11             ; LIAM_SYSCALL_GET_ARG
    int 0x80
    test eax, eax
    js .arg_failed

    mov edi, esi
    add edi, .path_buffer - .get_eip
    mov ebp, eax
    jmp .print_start

.use_default_path:
    mov edi, esi
    add edi, .default_path - .get_eip
    mov ebp, .default_path_len

.print_start:
    mov ebx, esi
    add ebx, .start_prefix - .get_eip
    mov ecx, .start_prefix_len
    call .write_buffer

    mov ebx, edi
    mov ecx, ebp
    call .write_buffer

    mov ebx, esi
    add ebx, .newline - .get_eip
    mov ecx, .newline_len
    call .write_buffer

    mov ebx, edi
    mov ecx, 0              ; flags
    mov edx, 0
    mov eax, 6              ; LIAM_SYSCALL_OPEN
    int 0x80
    test eax, eax
    js .open_failed

    mov edi, eax            ; fd

    mov ebx, edi            ; fd
    mov ecx, esi
    add ecx, .read_buffer - .get_eip
    mov edx, .read_buffer_len
    mov eax, 7              ; LIAM_SYSCALL_READ
    int 0x80
    test eax, eax
    js .read_failed

    mov ebp, eax            ; bytes read

    cmp ebp, 0
    je .close_file

    mov ebx, esi
    add ebx, .read_buffer - .get_eip
    mov ecx, ebp
    call .write_buffer

.close_file:
    mov ebx, edi
    mov ecx, 0
    mov edx, 0
    mov eax, 8              ; LIAM_SYSCALL_CLOSE
    int 0x80
    test eax, eax
    js .close_failed

    mov ebx, 0              ; exit code
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

.arg_failed:
    mov ebx, esi
    add ebx, .arg_failed_message - .get_eip
    mov ecx, .arg_failed_message_len
    jmp .fatal_write

.open_failed:
    mov ebx, esi
    add ebx, .open_failed_message - .get_eip
    mov ecx, .open_failed_message_len
    jmp .fatal_write

.read_failed:
    mov ebx, esi
    add ebx, .read_failed_message - .get_eip
    mov ecx, .read_failed_message_len
    jmp .fatal_write

.close_failed:
    mov ebx, esi
    add ebx, .close_failed_message - .get_eip
    mov ecx, .close_failed_message_len

.fatal_write:
    call .write_buffer

    mov ebx, 1              ; exit code
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

    ud2

.write_buffer:
    push eax
    push edx

    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

    pop edx
    pop eax
    ret

.start_prefix:
    db 'cat: '
.start_prefix_len equ $ - .start_prefix

.arg_failed_message:
    db 'cat: failed to read argv[1]', 10
.arg_failed_message_len equ $ - .arg_failed_message

.open_failed_message:
    db 'cat: open failed', 10
.open_failed_message_len equ $ - .open_failed_message

.read_failed_message:
    db 'cat: read failed', 10
.read_failed_message_len equ $ - .read_failed_message

.close_failed_message:
    db 'cat: close failed', 10
.close_failed_message_len equ $ - .close_failed_message

.default_path:
    db '/etc/os-release', 0
.default_path_end:
.default_path_len equ .default_path_end - .default_path - 1

.newline:
    db 10
.newline_len equ $ - .newline

.path_buffer:
    times 128 db 0
.path_buffer_len equ 128

.read_buffer:
    times 256 db 0
.read_buffer_len equ 255