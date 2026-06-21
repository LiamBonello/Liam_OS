global paging_load_directory
global paging_enable
global paging_read_cr0
global paging_read_cr2
global paging_read_cr3
global paging_invalidate_page

section .text
bits 32

paging_load_directory:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

paging_enable:
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret

paging_read_cr0:
    mov eax, cr0
    ret

paging_read_cr2:
    mov eax, cr2
    ret

paging_read_cr3:
    mov eax, cr3
    ret

paging_invalidate_page:
    mov eax, [esp + 4]
    invlpg [eax]
    ret