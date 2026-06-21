# Liam_OS x86_64 Architecture Scaffold

This directory contains the staged x86_64 kernel path. The i386 path in `core/kernel/arch/i386/` remains the stable baseline while this directory grows in coherent, testable stages.

## Current status

The x86_64 path can build an experimental ELF64 kernel artifact through:

```sh
cd core
make x86_64-kernel
```

The artifact is written to:

```txt
core/build/x86_64/kernel.elf
```

It can also build and run an experimental Multiboot2 ISO that enters long mode, calls a freestanding x86_64 C entry, installs an early exception IDT, loads a maintained GDT with a 64-bit TSS descriptor, builds an architecture boot context, parses boot information, computes an early PMM plan, initializes an isolated bounded PMM page allocator, captures bootstrap paging state, prepares bootstrap IST stacks, and writes VGA/serial diagnostics:

```sh
cd core
make x86_64-iso
make x86_64-run
```

The ISO is written to:

```txt
core/build/x86_64/liam_os_x86_64.iso
```

Expected screen messages include:

```txt
Liam_OS x86_64 kernel diagnostics
Stage: descriptor load + paging + PMM
Multiboot2: ok
Bootloader: ...
Boot info pointer: 0x........
Boot info bytes: ...
Memory map entries: ...
Usable bytes: 0x................
IDT: exceptions installed
Kernel start: 0x................
Kernel end: 0x................
Kernel bytes: 0x................
Identity map bytes: 0x................
PMM usable regions: ...
PMM first page: 0x................
PMM pages: 0x................
PMM bytes: 0x................
Reserved below: 0x................
PMM tracked pages: ...
PMM smoke page: 0x................
PMM smoke free: 1
Paging huge pages: 512
GDT/TSS loaded ok: 1
```

If an early CPU exception fires after IDT installation, the handler prints the exception name over VGA and the full vector/error-code diagnostics over serial.

`make x86_64-run` also routes COM1 serial output to the terminal, including full paging table diagnostics for CR3, PML4, PDPT, PD, huge-page count, identity-map span, GDTR base/limit, active selectors, current task-register selector, descriptor values, TSS base/limit, and planned IST stack addresses.

This is not the full x86_64 kernel yet. The current path proves an assembly handoff into long mode, a minimal C entry, early boot diagnostics, Multiboot2 tag parsing, an early CPU exception IDT, a C-built maintained GDT, a loaded x86_64 TSS descriptor, an architecture-owned boot context, a planning PMM view over retained memory-map regions, an isolated physical page allocator smoke test, C-visible bootstrap paging-state diagnostics, and a C-visible bootstrap TSS/IST plan. It does not wire exception gates to IST entries yet, initialize IRQ routing, APIC/PIC/PIT, the shared paging subsystem, heap, processes, syscalls, or userspace.

## Milestones

1. Add x86_64 build metadata without changing the default i386 build. Done.
2. Add a separate linker script and boot objects for the x86_64 path. Done for the first experimental path.
3. Enter long mode from a 32-bit bootstrap. Started.
4. Bring up a minimal 64-bit C console path. Started.
5. Parse Multiboot2 boot information and memory map. Started.
6. Add x86_64 GDT/IDT/interrupt handling. Started with a maintained GDT, loaded TSS, bootstrap IST planning, and exception IDT.
7. Port paging and memory layout to 64-bit addresses. Started with boot-context layout, PMM planning diagnostics, an isolated PMM allocator smoke test, and bootstrap paging-state diagnostics.
8. Revisit syscalls and userspace after the 64-bit kernel path is stable.

## Guardrails

- Do not copy i386 files and rename them as if that made them 64-bit safe.
- Keep i386 bootable until x86_64 has its own verified boot path.
- Keep architecture-specific code under architecture-specific directories.
- Promote shared abstractions only after both i386 and x86_64 need the same contract.
