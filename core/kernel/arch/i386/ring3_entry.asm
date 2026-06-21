global ring3_enter_user_mode
global ring3_resume_kernel_from_user
global ring3_syscall_stub

extern ring3_syscall_entry

section .text
bits 32

ring3_enter_user_mode:
    mov eax, [esp + 4]      ; user eip
    mov edx, [esp + 8]      ; user esp

    mov [ring3_saved_kernel_esp], esp
    mov [ring3_saved_kernel_ebp], ebp

    push dword 0x23         ; user data selector
    push edx                ; user stack

    pushfd
    pop ecx
    or ecx, 0x00000200      ; IF=1 for user mode
    push ecx

    push dword 0x1B         ; user code selector
    push eax                ; user eip
    iretd

ring3_kernel_return:
    ret

ring3_resume_kernel_from_user:
    ; The syscall exit path jumps here from inside ring3_syscall_entry instead of
    ; returning through ring3_syscall_stub. The stub executes cli before entering C,
    ; so interrupts must be re-enabled before returning to kernel C code. Without
    ; this, the shell remains visible but keyboard IRQs are never delivered.
    mov esp, [ring3_saved_kernel_esp]
    mov ebp, [ring3_saved_kernel_ebp]
    sti
    jmp ring3_kernel_return

ring3_syscall_stub:
    cli
    pushad

    push dword [esp + 20]   ; user edx: arg3
    push dword [esp + 28]   ; user ecx: arg2
    push dword [esp + 24]   ; user ebx: arg1
    push dword [esp + 40]   ; user eax: syscall number
    call ring3_syscall_entry
    add esp, 16

    mov [esp + 28], eax     ; return value becomes user eax after popad

    popad
    sti
    iretd

section .bss
align 4
ring3_saved_kernel_esp:
    resd 1
ring3_saved_kernel_ebp:
    resd 1