# Liam_OS

Liam_OS is split into two coordinated tracks:

1. **Liam_OS Core**: the original from-scratch kernel project in `core/`.
2. **Liam_OS Desktop**: the future market-facing Linux-based desktop distribution plan in `desktop/` and `docs/product/`.

This split keeps the original kernel work intact while giving the commercial desktop product a realistic path to shipping.

## Current status

`core/` is the active buildable project. It is a freestanding i386 kernel with GRUB Multiboot booting, protected-mode setup, interrupts, paging, memory management, basic scheduling/process abstractions, VFS/initramfs foundations, flat userspace image loading, and an early syscall path.

An x86_64 architecture path is now scaffolded for staged migration planning. It can build an experimental ELF64 kernel artifact and an experimental Multiboot2 long-mode ISO. The default stable Core build remains i386.

`desktop/` is intentionally a planning/skeleton area. It should not pretend to be an installable commercial OS yet.

## Build Liam_OS Core

On Ubuntu/WSL, install the expected tools:

```sh
sudo apt update
sudo apt install nasm clang lld grub-pc-bin grub-common xorriso qemu-system-x86 make mtools
```

Then build and run the current i386 Core path:

```sh
make check-tools
make
make run
```

Equivalent direct commands:

```sh
cd core
make check-tools
make
make run
```

The generated ISO is written to:

```txt
core/build/liam_os.iso
```

To see the staged x86_64 migration status:

```sh
cd core
make x86_64-info
```

To build the current experimental x86_64 ELF64 artifact:

```sh
cd core
make x86_64-kernel
```

The generated artifact is written to:

```txt
core/build/x86_64/kernel.elf
```

To build and run the current experimental x86_64 long-mode ISO:

```sh
cd core
make x86_64-iso
make x86_64-run
```

The generated experimental ISO is written to:

```txt
core/build/x86_64/liam_os_x86_64.iso
```

## Repository layout

```txt
core/              Original custom kernel project
  kernel/          Kernel source
    arch/i386/     Current bootable 32-bit architecture path
    arch/x86_64/   Future 64-bit architecture scaffold
  boot/            GRUB boot configuration
  docs/            Core architecture notes
  Makefile         Core build

desktop/           Future Linux-based commercial desktop distribution skeleton
  apps/            Liam-branded desktop app plans
  iso/             ISO/remix build planning
  profiles/        Gamer/developer/creator profile definitions
  themes/          Visual identity and theming plan

docs/
  product/         Product strategy and roadmap
  commercial/      Licensing/commercialization notes
  core/            Cross-track kernel roadmap notes

scripts/           Repo utility scripts
```

## Strategic direction

The recommended product strategy is:

- Keep `core/` as the original Liam kernel research/development track.
- Build the first marketable Liam_OS Desktop on a Linux base with custom UX, branding, tools, profiles, and installer flow.
- Do not claim the desktop edition is built from scratch if it uses Linux. Claim it accurately as a custom Linux-based operating system.

## Naming

Recommended product naming:

```txt
Liam_OS Core       Original custom kernel
Liam_OS Desktop    Market-facing Linux-based desktop edition
```
