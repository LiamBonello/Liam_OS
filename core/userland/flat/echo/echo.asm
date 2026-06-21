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

    mov [esi + .argc_value - .get_eip], eax

    cmp eax, 2
    jb .print_newline

    mov dword [esi + .arg_index - .get_eip], 1

.next_arg:
    mov eax, [esi + .arg_index - .get_eip]
    cmp eax, [esi + .argc_value - .get_eip]
    jae .print_newline

    cmp eax, 1
    je .read_arg

    mov ebx, esi
    add ebx, .space - .get_eip
    mov ecx, .space_len
    call .write_buffer

.read_arg:
    mov ebx, [esi + .arg_index - .get_eip]
    mov ecx, esi
    add ecx, .arg_buffer - .get_eip
    mov edx, .arg_buffer_len
    mov eax, 11             ; LIAM_SYSCALL_GET_ARG
    int 0x80
    test eax, eax
    js .arg_failed

    mov ebx, esi
    add ebx, .arg_buffer - .get_eip
    mov ecx, eax
    call .write_buffer

    inc dword [esi + .arg_index - .get_eip]
    jmp .next_arg

.print_newline:
    mov ebx, esi
    add ebx, .newline - .get_eip
    mov ecx, .newline_len
    call .write_buffer

    mov ebx, 0
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

.arg_failed:
    mov ebx, esi
    add ebx, .arg_failed_message - .get_eip
    mov ecx, .arg_failed_message_len
    call .write_buffer

    mov ebx, 1
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

.space:
    db ' '
.space_len equ $ - .space

.newline:
    db 10
.newline_len equ $ - .newline

.arg_failed_message:
    db 'echo: failed to read argument', 10
.arg_failed_message_len equ $ - .arg_failed_message

.argc_value:
    dd 0

.arg_index:
    dd 0

.arg_buffer:
    times 128 db 0
.arg_buffer_len equ 128