bits 32

_start:
    call .get_eip

.get_eip:
    pop esi

    mov ebx, esi
    add ebx, .prefix - .get_eip
    mov ecx, .prefix_len
    call .write_buffer

    mov ebx, 0
    mov ecx, 0
    mov edx, 0
    mov eax, 3              ; LIAM_SYSCALL_GET_TICKS
    int 0x80

    call .write_u32_decimal

    mov ebx, esi
    add ebx, .newline - .get_eip
    mov ecx, .newline_len
    call .write_buffer

    mov ebx, 0              ; exit code
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

.prefix:
    db 'ticks: '
.prefix_len equ $ - .prefix

.newline:
    db 10
.newline_len equ $ - .newline

.number_buffer:
    times 16 db 0
.number_buffer_end: