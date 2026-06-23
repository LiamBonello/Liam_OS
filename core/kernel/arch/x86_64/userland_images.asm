global x86_64_user_init_image_start
global x86_64_user_init_image_end

section .rodata
align 16
x86_64_user_init_image_start:
    incbin "build/x86_64/userland/init.elf"
x86_64_user_init_image_end:
