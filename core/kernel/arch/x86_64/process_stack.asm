bits 64

global x86_64_process_enter_stack

section .text

; void x86_64_process_enter_stack(u64 stack_top,
;                                  x86_64_process_entry_t entry,
;                                  void *arg)
; rdi = stack_top, rsi = entry, rdx = arg
x86_64_process_enter_stack:
    push rbx
    mov rbx, rsp

    mov rsp, rdi
    and rsp, -16
    mov rdi, rdx
    call rsi

    mov rsp, rbx
    pop rbx
    ret
