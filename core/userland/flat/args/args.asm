bits 32

_start:
    call .get_eip

.get_eip:
    pop esi

    mov ebx, esi
    add ebx, .header - .get_eip
    mov ecx, .header_len
    call .write_buffer

    mov ebx, 0
    mov ecx, 0
    mov edx, 0
    mov eax, 10             ; LIAM_SYSCALL_GET_ARGC
    int 0x80

    mov [esi + .argc_value - .get_eip], eax

    mov ebx, esi
    add ebx, .argc_prefix - .get_eip
    mov ecx, .argc_prefix_len
    call .write_buffer

    mov eax, [esi + .argc_value - .get_eip]
    call .write_u32_decimal

    mov ebx, esi
    add ebx, .newline - .get_eip
    mov ecx, .newline_len
    call .write_buffer

    mov dword [esi + .arg_index - .get_eip], 0

.next_arg:
    mov eax, [esi + .arg_index - .get_eip]
    cmp eax, [esi + .argc_value - .get_eip]
    jae .done

    mov ebx, esi
    add ebx, .argv_prefix - .get_eip
    mov ecx, .argv_prefix_len
    call .write_buffer

    mov eax, [esi + .arg_index - .get_eip]
    call .write_u32_decimal

    mov ebx, esi
    add ebx, .argv_middle - .get_eip
    mov ecx, .argv_middle_len
    call .write_buffer

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

    mov ebx, esi
    add ebx, .quote_newline - .get_eip
    mov ecx, .quote_newline_len
    call .write_buffer

    inc dword [esi + .arg_index - .get_eip]
    jmp .next_arg

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

.done:
    mov ebx, 0
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

.header:
    db 'args: userspace argv inspection', 10
.header_len equ $ - .header

.argc_prefix:
    db 'argc='
.argc_prefix_len equ $ - .argc_prefix

.argv_prefix:
    db 'argv['
.argv_prefix_len equ $ - .argv_prefix

.argv_middle:
    db ']="'
.argv_middle_len equ $ - .argv_middle

.quote_newline:
    db '"', 10
.quote_newline_len equ $ - .quote_newline

.arg_failed_message:
    db 'args: failed to read argument', 10
.arg_failed_message_len equ $ - .arg_failed_message

.newline:
    db 10
.newline_len equ $ - .newline

.argc_value:
    dd 0

.arg_index:
    dd 0

.number_buffer:
    times 16 db 0
.number_buffer_end:

.arg_buffer:
    times 128 db 0
.arg_buffer_len equ 128