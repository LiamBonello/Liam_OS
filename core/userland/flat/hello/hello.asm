bits 32

_start:
    call .get_eip

.get_eip:
    pop esi

    mov ebx, esi
    add ebx, .hello_message - .get_eip
    mov ecx, .hello_message_len
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

    mov ebx, 0              ; exit code
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

    ud2

.hello_message:
    db 'hello: Liam_OS userland-built program executed', 10
.hello_message_len equ $ - .hello_message