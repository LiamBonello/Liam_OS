# Liam_OS Core

This directory contains the original from-scratch Liam_OS kernel project.

## Build

```sh
make check-tools
make
make run
```

The ISO is generated at:

```txt
build/liam_os.iso
```

## Current architecture

- Architecture: i386.
- Boot: GRUB Multiboot.
- Kernel mode: 32-bit protected mode.
- Current user boundary: ring 3 transition with `int 0x80` syscall dispatch.
- Storage model: early static initramfs/VFS foundation.
- Executable model: flat binary loader; ELF loading is a future milestone.

See `docs/ARCHITECTURE.md` for details.
