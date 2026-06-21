global _start
extern x86_64_start

MULTIBOOT2_MAGIC equ 0xE85250D6
MULTIBOOT2_ARCH_I386 equ 0
MULTIBOOT2_HEADER_LENGTH equ multiboot2_header_end - multiboot2_header_start
MULTIBOOT2_CHECKSUM equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH_I386 + MULTIBOOT2_HEADER_LENGTH)

CR0_PG equ 0x80000000
CR4_PAE equ 0x00000020
EFER_MSR equ 0xC0000080
EFER_LME equ 0x00000100
PAGE_PRESENT equ 0x001
PAGE_WRITABLE equ 0x002
PAGE_HUGE equ 0x080
PAGE_SIZE_2M equ 0x200000
CODE64_SEL equ gdt64_code - gdt64_start
DATA64_SEL equ gdt64_data - gdt64_start

section .multiboot
align 8
multiboot2_header_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH_I386
    dd MULTIBOOT2_HEADER_LENGTH
    dd MULTIBOOT2_CHECKSUM

    dw 0
    dw 0
    dd 8
multiboot2_header_end:

section .text
bits 32
_start:
    cli
    mov [boot_magic], eax
    mov [boot_info], ebx
    mov esp, boot_stack_top

    call setup_identity_paging

    lgdt [gdt64_descriptor]

    mov eax, cr4
    or eax, CR4_PAE
    mov cr4, eax

    mov eax, pml4_table
    mov cr3, eax

    mov ecx, EFER_MSR
    rdmsr
    or eax, EFER_LME
    wrmsr

    mov eax, cr0
    or eax, CR0_PG
    mov cr0, eax

    jmp CODE64_SEL:long_mode_entry

setup_identity_paging:
    mov eax, pdpt_table
    or eax, PAGE_PRESENT | PAGE_WRITABLE
    mov [pml4_table], eax
    mov dword [pml4_table + 4], 0

    mov eax, pd_table
    or eax, PAGE_PRESENT | PAGE_WRITABLE
    mov [pdpt_table], eax
    mov dword [pdpt_table + 4], 0

    mov edi, pd_table
    mov ecx, 512
    xor eax, eax

.map_pd_entry:
    mov edx, eax
    or edx, PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE
    mov [edi], edx
    mov dword [edi + 4], 0
    add eax, PAGE_SIZE_2M
    add edi, 8
    loop .map_pd_entry
    ret

bits 64
long_mode_entry:
    mov ax, DATA64_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, boot_stack_top
    mov edi, [rel boot_magic]
    mov esi, [rel boot_info]

    call x86_64_start

.hang:
    cli
    hlt
    jmp .hang

section .rodata
align 8
gdt64_start:
    dq 0x0000000000000000
gdt64_code:
    dq 0x00209A0000000000
gdt64_data:
    dq 0x0000920000000000
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start

section .bss
alignb 4096
pml4_table:
    resq 512
pdpt_table:
    resq 512
pd_table:
    resq 512

alignb 4
boot_magic:
    resd 1
boot_info:
    resd 1

alignb 16
boot_stack_bottom:
    resb 16384
boot_stack_top:
