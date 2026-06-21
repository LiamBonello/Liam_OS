bits 32

_start:
    call .get_eip

.get_eip:
    pop esi

    mov ebx, esi
    add ebx, .help_message - .get_eip
    mov ecx, .help_message_len
    mov edx, 0
    mov eax, 2              ; LIAM_SYSCALL_WRITE
    int 0x80

    mov ebx, 0              ; exit code
    mov ecx, 0
    mov edx, 0
    mov eax, 1              ; LIAM_SYSCALL_EXIT
    int 0x80

    ud2

.help_message:
    db 'Liam_OS userspace help', 10
    db 10
    db 'Available user programs:', 10
    db '  /bin/about       Show Liam_OS version information', 10
    db '  /bin/args        Show argc and argv values', 10
    db '  /bin/cat [path]  Print a file; defaults to /etc/os-release', 10
    db '  /bin/clear       Clear the console', 10
    db '  /bin/echo ...    Print arguments', 10
    db '  /bin/hello       Run a basic userspace hello program', 10
    db '  /bin/hello-elf   Run the ELF32 hello test program', 10
    db '  /bin/help        Show this help text', 10
    db '  /bin/os-release  Print OS release metadata', 10
    db '  /bin/pid         Print the current process ID', 10
    db '  /bin/sh          Run userspace shell syscall demo', 10
    db '  /bin/syscheck    Run syscall smoke tests', 10
    db '  /bin/ticks       Print kernel tick count', 10
    db '  /bin/uptime      Print system uptime ticks', 10
    db 10
    db 'Shell commands:', 10
    db '  images           List registered user images', 10
    db '  exec <path>      Execute a userspace program', 10
    db '  ps               List process table entries', 10
    db '  proc-status      Show process subsystem status', 10
    db '  wait-last        Wait for the most recent process result', 10
    db '  run-ready        Run the next READY process', 10
    db '  run-pid <pid>    Run a READY process by PID', 10
.help_message_len equ $ - .help_message