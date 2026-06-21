section .rodata
bits 32

global ring3_user_image_start
global ring3_user_image_end
global ring3_hello_image_start
global ring3_hello_image_end
global ring3_about_image_start
global ring3_about_image_end
global ring3_pid_image_start
global ring3_pid_image_end
global ring3_ticks_image_start
global ring3_ticks_image_end
global ring3_syscheck_image_start
global ring3_syscheck_image_end
global ring3_sysbadptr_image_start
global ring3_sysbadptr_image_end
global ring3_os_release_image_start
global ring3_os_release_image_end
global ring3_cat_image_start
global ring3_cat_image_end
global ring3_clear_image_start
global ring3_clear_image_end
global ring3_help_image_start
global ring3_help_image_end
global ring3_uptime_image_start
global ring3_uptime_image_end
global ring3_args_image_start
global ring3_args_image_end
global ring3_echo_image_start
global ring3_echo_image_end
global ring3_sh_image_start
global ring3_sh_image_end

align 16
ring3_user_image_start:
    incbin "build/userland/init.bin"
ring3_user_image_end:

align 16
ring3_hello_image_start:
    incbin "build/userland/hello.bin"
ring3_hello_image_end:

align 16
ring3_about_image_start:
    incbin "build/userland/about.bin"
ring3_about_image_end:

align 16
ring3_pid_image_start:
    incbin "build/userland/pid.bin"
ring3_pid_image_end:

align 16
ring3_ticks_image_start:
    incbin "build/userland/ticks.bin"
ring3_ticks_image_end:

align 16
ring3_syscheck_image_start:
    incbin "build/userland/syscheck.bin"
ring3_syscheck_image_end:

align 16
ring3_sysbadptr_image_start:
    incbin "build/userland/sysbadptr.bin"
ring3_sysbadptr_image_end:

align 16
ring3_os_release_image_start:
    incbin "build/userland/os-release.bin"
ring3_os_release_image_end:

align 16
ring3_cat_image_start:
    incbin "build/userland/cat.bin"
ring3_cat_image_end:

align 16
ring3_clear_image_start:
    incbin "build/userland/clear.bin"
ring3_clear_image_end:

align 16
ring3_help_image_start:
    incbin "build/userland/help.bin"
ring3_help_image_end:

align 16
ring3_uptime_image_start:
    incbin "build/userland/uptime.bin"
ring3_uptime_image_end:

align 16
ring3_args_image_start:
    incbin "build/userland/args.bin"
ring3_args_image_end:

align 16
ring3_echo_image_start:
    incbin "build/userland/echo.bin"
ring3_echo_image_end:

align 16
ring3_sh_image_start:
    incbin "build/userland/sh.bin"
ring3_sh_image_end: