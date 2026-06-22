# Liam_OS

Liam_OS is split into two coordinated tracks:

1. **Liam_OS Core**: the original from-scratch kernel project in `core/`.
2. **Liam_OS Desktop**: the future market-facing Linux-based desktop distribution plan in `desktop/` and `docs/product/`.

This split keeps the original kernel work intact while giving the commercial desktop product a realistic path to shipping.

## Current status

`core/` is the active buildable project. It is a freestanding i386 kernel with GRUB Multiboot booting, protected-mode setup, interrupts, paging, memory management, basic scheduling/process abstractions, VFS/initramfs foundations, flat userspace image loading, and an early syscall path.

An x86_64 architecture path is scaffolded for staged migration planning. It can build an experimental ELF64 kernel artifact and an experimental Multiboot2 long-mode ISO with an early freestanding C entry, VGA diagnostics, COM1 serial output, parsed Multiboot2 boot information, bounded command-line parsing, CPU capability baseline diagnostics, early CPU exception IDT diagnostics, guarded IRQ policy diagnostics, legacy IRQ gates for vectors 32-47, legacy PIC remapping, an opt-in software IRQ gate self-test, an opt-in invalid-opcode exception self-test, page-fault CR2/error-code diagnostics, exception panic-halt readiness diagnostics, dedicated NMI/double-fault/page-fault IST routing, a maintained GDT with a loaded TSS descriptor, bootstrap IST planning diagnostics, a bounded PIT timer smoke check that unmasks IRQ0 and enables interrupts on the normal x86_64 boot path, an architecture-owned boot context, memory-layout reporting, a higher-half/direct-map virtual memory plan, active C-owned x86_64 page tables for that plan, identity/direct-map/higher-half alias probes, higher-half assembly and C execution probes, a guarded higher-half handoff probe, a guarded higher-half runtime entry, early PMM planning diagnostics, an isolated early PMM page-allocator smoke check, and bootstrap paging-state diagnostics. The default stable Core build remains i386 until x86_64 reaches feature parity.

`desktop/` is intentionally a planning/skeleton area. It should not pretend to be an installable commercial OS yet.

## Build Liam_OS Core

On Ubuntu/WSL, install the expected tools:

```sh
sudo apt update
sudo apt install nasm clang lld grub-pc-bin grub-common xorriso qemu-system-x86 make mtools
```

Then build and run the current i386 Core path:

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

`make x86_64-run` routes COM1 serial output to the terminal. Expected x86_64 serial markers include `IRQ gates installed: 16`, `IRQ gates ok: 1`, `IRQ legacy PIC remapped: 1`, `PIT IRQ0 unmasked: 1`, `PIT waited ticks: 3`, `PIT timer ok: 1`, `Runtime entry ok: 1`, and `Desc/IST ok: 1`.

To build and run the opt-in x86_64 IRQ and invalid-opcode exception self-test ISO:

```sh
cd core
make x86_64-exception-test-iso
make x86_64-exception-test-run
```

The generated exception self-test ISO is written to:

```txt
core/build/x86_64/liam_os_x86_64_exception_test.iso
```

Expected serial markers include `IRQ self-test requested: 1`, `x86_64 IRQ received`, `IRQ vector: 0x0000000000000020`, `IRQ self-test delivered: 1`, `IRQ self-test returned: 1`, `Exception self-test requested: 1`, `Name: invalid opcode`, `Vector: 0x0000000000000006`, and `x86_64 panic halt`.

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