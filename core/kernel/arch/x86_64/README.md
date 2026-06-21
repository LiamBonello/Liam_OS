# Liam_OS x86_64 Architecture Scaffold

This directory contains the staged x86_64 kernel path. The i386 path in `core/kernel/arch/i386/` remains the stable baseline while this directory grows in small, testable stages.

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

It can also build and run an experimental Multiboot2 ISO that enters long mode, calls a freestanding x86_64 C entry, and writes VGA text:

```sh
cd core
make x86_64-iso
make x86_64-run
```

The ISO is written to:

```txt
core/build/x86_64/liam_os_x86_64.iso
```

Expected screen messages:

```txt
Liam_OS x86_64 C kernel entry online
Stage: long mode -> freestanding C
```

This is not the full x86_64 kernel yet. The current path proves an assembly handoff into long mode and a minimal C entry. It does not initialize the shared kernel runtime, interrupts, paging subsystem, heap, processes, syscalls, or userspace.

## Milestones

1. Add x86_64 build metadata without changing the default i386 build. Done.
2. Add a separate linker script and boot objects for the x86_64 path. Done for the first experimental path.
3. Enter long mode from a 32-bit bootstrap. Started.
4. Bring up a minimal 64-bit C console path. Started.
5. Add x86_64 GDT/IDT/interrupt handling.
6. Port paging and memory layout to 64-bit addresses.
7. Revisit syscalls and userspace after the 64-bit kernel path is stable.

## Guardrails

- Do not copy i386 files and rename them as if that made them 64-bit safe.
- Keep i386 bootable until x86_64 has its own verified boot path.
- Keep architecture-specific code under architecture-specific directories.
- Promote shared abstractions only after both i386 and x86_64 need the same contract.
