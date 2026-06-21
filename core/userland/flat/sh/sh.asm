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
    add ebx, .exec_message - .get_eip
    mov ecx, .exec_message_len
    call .write_buffer

    mov ebx, esi
    add ebx, .echo_path - .get_eip
    mov ecx, esi
    add ecx, .echo_args - .get_eip
    mov edx, 0
    mov eax, 12             ; LIAM_SYSCALL_EXEC
    int 0x80
    test eax, eax
    js .exec_failed

    mov edi, eax

    mov ebx, esi
    add ebx, .pid_prefix - .get_eip
    mov ecx, .pid_prefix_len
    call .write_buffer

    mov eax, edi
    call .write_u32_decimal

    mov ebx, esi
    add ebx, .newline - .get_eip
    mov ecx, .newline_len
    call .write_buffer

    mov ebx, esi
    add ebx, .done_message - .get_eip
    mov ecx, .done_message_len
    call .write_buffer

    mov ebx, 0
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

.exec_failed:
    mov ebx, esi
    add ebx, .exec_failed_message - .get_eip
    mov ecx, .exec_failed_message_len
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

.write_u32_decimal:
    push ebx
    push ecx
    push edx
    push edi

    mov edi, esi
    add edi, .number_buffer_end - .get_eip
    mov ecx, 0

    cmp eax, 0
    jne .convert_decimal_loop

    dec edi
    mov byte [edi], '0'
    mov ecx, 1
    jmp .write_decimal_digits

.convert_decimal_loop:
    mov ebx, 10

.next_decimal_digit:
    xor edx, edx
    div ebx
    add dl, '0'
    dec edi
    mov [edi], dl
    inc ecx
    test eax, eax
    jne .next_decimal_digit

.write_decimal_digits:
    mov ebx, edi
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

    pop edi
    pop edx
    pop ecx
    pop ebx
    ret

.banner:
    db 'Liam_OS userspace shell', 10
.banner_len equ $ - .banner

.exec_message:
    db 'sh: creating /bin/echo through LIAM_SYSCALL_EXEC', 10
.exec_message_len equ $ - .exec_message

.pid_prefix:
    db 'sh: child pid='
.pid_prefix_len equ $ - .pid_prefix

.done_message:
    db 'sh: child process created in ready state', 10
.done_message_len equ $ - .done_message

.exec_failed_message:
    db 'sh: exec syscall failed', 10
.exec_failed_message_len equ $ - .exec_failed_message

.echo_path:
    db '/bin/echo', 0

.echo_args:
    db 'hello from /bin/sh via syscall exec', 0

.newline:
    db 10
.newline_len equ $ - .newline

.number_buffer:
    times 16 db 0
.number_buffer_end: