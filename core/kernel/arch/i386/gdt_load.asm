global gdt_load
global gdt_load_tss

section .text
bits 32

gdt_load:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:gdt_flush

gdt_flush:
    ret

gdt_load_tss:
    mov ax, [esp + 4]
    ltr ax
    ret
