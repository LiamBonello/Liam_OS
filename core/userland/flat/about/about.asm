bits 32

_start:
    call .get_eip

.get_eip:
    pop esi

    mov ebx, esi
    add ebx, .about_message - .get_eip
    mov ecx, .about_message_len
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

    mov ebx, 0              ; exit code
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

    ud2

.about_message:
    db 'Liam_OS Core', 10
    db 'Version: 0.8.60-dev', 10
    db 'Architecture: i386', 10
    db 'Userspace: flat binaries, ELF32, and userland-built binaries', 10
.about_message_len equ $ - .about_message