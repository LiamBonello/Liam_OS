bits 64

section .text

global x86_64_higher_half_probe_entry
global x86_64_higher_half_probe_entry_end

x86_64_higher_half_probe_entry:
    mov eax, 0x48485031
    ret

x86_64_higher_half_probe_entry_end:
