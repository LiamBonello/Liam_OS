global irq_save
global irq_restore
global irq_disable
global irq_enable
global irq_are_enabled

section .text
bits 32

; uint32_t irq_save(void)
; Returns EFLAGS before disabling interrupts.
irq_save:
    pushfd
    pop eax
    cli
    ret

; void irq_restore(uint32_t flags)
; Restores IF according to saved EFLAGS.
irq_restore:
    mov eax, [esp + 4]
    push eax
    popfd
    ret

irq_disable:
    cli
    ret

irq_enable:
    sti
    ret

; uint8_t irq_are_enabled(void)
; Returns 1 if IF bit is set, otherwise 0.
irq_are_enabled:
    pushfd
    pop eax
    and eax, 0x00000200
    cmp eax, 0
    jne .enabled
    mov eax, 0
    ret

.enabled:
    mov eax, 1
    ret