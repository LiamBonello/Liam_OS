bits 32

_start:
    call .get_eip

.get_eip:
    pop esi

    mov ebx, esi
    add ebx, .started_message - .get_eip
    mov ecx, .started_message_len
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

    mov ebx, esi
    add ebx, .stat_buffer - .get_eip
    push ebx                ; save stat buffer pointer

    mov ebx, esi
    add ebx, .os_release_path - .get_eip
    pop ecx
    mov edx, 0
    mov eax, 9              ; LIAM_SYSCALL_STAT
    int 0x80
    test eax, eax
    js .stat_failed

    mov ebx, esi
    add ebx, .stat_ok_message - .get_eip
    mov ecx, .stat_ok_message_len
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

    mov ebx, edi            ; fd
    mov ecx, esi
    add ecx, .read_buffer - .get_eip
    mov edx, .read_buffer_len
    mov eax, 7              ; LIAM_SYSCALL_READ
    int 0x80
    test eax, eax
    js .read_failed
    mov ebp, eax            ; bytes read

    mov ebx, esi
    add ebx, .release_message - .get_eip
    mov ecx, .release_message_len
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

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

    mov ebx, esi
    add ebx, .ready_message - .get_eip
    mov ecx, .ready_message_len
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

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

.stat_failed:
    mov ebx, esi
    add ebx, .stat_failed_message - .get_eip
    mov ecx, .stat_failed_message_len
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

.started_message:
    db 'init: userspace process started', 10
.started_message_len equ $ - .started_message

.stat_ok_message:
    db 'init: stat /etc/os-release ok', 10
.stat_ok_message_len equ $ - .stat_ok_message

.release_message:
    db 'init: reading /etc/os-release', 10
.release_message_len equ $ - .release_message

.ready_message:
    db 'init: filesystem syscalls online', 10
.ready_message_len equ $ - .ready_message

.open_failed_message:
    db 'init: open /etc/os-release failed', 10
.open_failed_message_len equ $ - .open_failed_message

.stat_failed_message:
    db 'init: stat /etc/os-release failed', 10
.stat_failed_message_len equ $ - .stat_failed_message

.read_failed_message:
    db 'init: read /etc/os-release failed', 10
.read_failed_message_len equ $ - .read_failed_message

.close_failed_message:
    db 'init: close /etc/os-release failed', 10
.close_failed_message_len equ $ - .close_failed_message

.os_release_path:
    db '/etc/os-release', 0

align 4
.stat_buffer:
    times 20 db 0

.read_buffer:
    times 256 db 0
.read_buffer_len equ 255