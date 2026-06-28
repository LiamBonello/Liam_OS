global x86_64_user_init_image_start
global x86_64_user_init_image_end
global x86_64_user_hello_image_start
global x86_64_user_hello_image_end
global x86_64_user_inputd_image_start
global x86_64_user_inputd_image_end
global x86_64_user_storaged_image_start
global x86_64_user_storaged_image_end
global x86_64_user_sysinfo_image_start
global x86_64_user_sysinfo_image_end
global x86_64_user_timed_image_start
global x86_64_user_timed_image_end
global x86_64_user_windowd_image_start
global x86_64_user_windowd_image_end

section .rodata
align 16
x86_64_user_init_image_start:
    incbin "build/x86_64/userland/init.elf"
x86_64_user_init_image_end:

align 16
x86_64_user_hello_image_start:
    incbin "build/x86_64/userland/hello.elf"
x86_64_user_hello_image_end:

align 16
x86_64_user_inputd_image_start:
    incbin "build/x86_64/userland/inputd.elf"
x86_64_user_inputd_image_end:

align 16
x86_64_user_storaged_image_start:
    incbin "build/x86_64/userland/storaged.elf"
x86_64_user_storaged_image_end:

align 16
x86_64_user_sysinfo_image_start:
    incbin "build/x86_64/userland/sysinfo.elf"
x86_64_user_sysinfo_image_end:

align 16
x86_64_user_timed_image_start:
    incbin "build/x86_64/userland/timed.elf"
x86_64_user_timed_image_end:

align 16
x86_64_user_windowd_image_start:
    incbin "build/x86_64/userland/windowd.elf"
x86_64_user_windowd_image_end:
