bits 32

_start:
    call .get_eip

.get_eip:
    pop esi

    mov ebx, esi
    add ebx, .clear_sequence - .get_eip
    mov ecx, .clear_sequence_len
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

    mov ebx, 0              ; exit code
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

    ud2

.clear_sequence:
    db 27, '[2J', 27, '[H'
.clear_sequence_len equ $ - .clear_sequence