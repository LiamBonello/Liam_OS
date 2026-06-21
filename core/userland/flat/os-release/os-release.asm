bits 32

_start:
    call .get_eip

.get_eip:
    pop esi

    mov ebx, esi
    add ebx, .start_message - .get_eip
    mov ecx, .start_message_len
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

    mov ebx, esi
    add ebx, .os_release_path - .get_eip
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
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

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
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

    mov ebx, 1              ; exit code
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

    ud2

.start_message:
    db 'os-release: reading /etc/os-release', 10
.start_message_len equ $ - .start_message

.open_failed_message:
    db 'os-release: open failed', 10
.open_failed_message_len equ $ - .open_failed_message

.read_failed_message:
    db 'os-release: read failed', 10
.read_failed_message_len equ $ - .read_failed_message

.close_failed_message:
    db 'os-release: close failed', 10
.close_failed_message_len equ $ - .close_failed_message

.os_release_path:
    db '/etc/os-release', 0

.read_buffer:
    times 256 db 0
.read_buffer_len equ 255