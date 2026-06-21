# Liam_OS Core 0.1 Boot Milestone

This milestone confirms that Liam_OS Core can boot into a working early kernel and userspace environment.

## Confirmed behavior

- Boots through GRUB in QEMU
- Initializes core kernel subsystems
- Initializes interrupts, timer, keyboard, scheduler, paging, heap, and syscalls
- Starts `/sbin/init` from the initramfs
- Reads `/etc/os-release`
- Exits the initial userspace process cleanly
- Enters the Liam OS shell

## Current userspace programs

- `/sbin/init`
- `/bin/hello`

## Current shell validation commands

```text
ls /
ls /bin
stat /bin/hello
exec /bin/hello
exec-status
images
fs-status