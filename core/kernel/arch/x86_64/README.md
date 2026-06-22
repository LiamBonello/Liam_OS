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

It can also build and run an experimental Multiboot2 ISO that enters long mode, calls a freestanding x86_64 C entry, reports a required CPU capability baseline, loads a maintained GDT with a 64-bit TSS descriptor, installs an early exception IDT after the TSS is ready, routes NMI through IST2, double fault through IST1, and page fault through IST3, remaps the legacy PIC, installs guarded legacy IRQ gates, runs bounded PIT and keyboard IRQ diagnostics on the normal boot path, parses bounded Multiboot2 boot information, builds an architecture boot context, initializes an isolated bounded PMM page allocator, builds PMM-backed C-owned page tables, switches CR3 to those tables, validates identity/direct-map/higher-half aliases, proves guarded higher-half execution and runtime-entry calls, initializes a PMM-backed direct-map early heap, and runs an architecture-local process scheduler smoke check with heap-backed kernel stacks.

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
Stage: x86_64 early heap
Multiboot2: ok
CPU baseline ok: 1
IDT IST gates: 1
PMM smoke free: 1
Heap smoke ok: 1
Process smoke ok: 1
Desc/IST ok: 1
```

`make x86_64-run` routes COM1 serial output to the terminal. Key serial markers include:

```txt
PIT timer ok: 1
Keyboard ready ok: 1
Paging builder PMM backed: 1
Paging builder table pages: 8
Paging builder ok: 1
Heap PMM backed: 1
Heap direct map ok: 1
Heap smoke ok: 1
Process created: 2
Process ready before run: 2
Process run count: 2
Process exited: 2
Process failed: 0
Process stack allocations: 2
Process worker A count: 1
Process worker B count: 1
Process smoke ok: 1
Runtime entry ok: 1
Desc/IST ok: 1
```

If an early CPU exception fires after IDT installation, the handler prints the exception name over VGA and the full vector/error-code diagnostics over serial. Page faults additionally report CR2 plus decoded present/write/user/reserved-bit/instruction-fetch error-code flags. All exceptions terminate through the shared panic halt helper, which announces the `cli; hlt` halt mode before stopping the CPU.

This is not the full x86_64 kernel yet. The current path proves the boot/descriptor/IRQ/memory/heap/process smoke foundations, but it does not yet provide real x86_64 context switching, process address spaces, syscalls, ELF64 loading, isolated userland, or a shell.

## Manual validation

Use this path for the normal x86_64 milestone:

```sh
cd core
make clean
make x86_64-info
make x86_64-kernel
make x86_64-iso
make x86_64-run
```

Check the serial output for:

```txt
PIT timer ok: 1
Keyboard ready ok: 1
Heap smoke ok: 1
Process created: 2
Process ready before run: 2
Process run count: 2
Process exited: 2
Process failed: 0
Process stack allocations: 2
Process stack alignment ok: 1
Process worker A count: 1
Process worker B count: 1
Process smoke ok: 1
Runtime entry ok: 1
Paging activation ok: 1
Paging probes ok: 1
Desc/IST ok: 1
```

Use this separate opt-in path for the x86_64 IRQ and exception self-test:

```sh
cd core
make clean
make x86_64-exception-test-iso
make x86_64-exception-test-run
```

Check the serial output for:

```txt
IRQ self-test requested: 1
x86_64 IRQ received
IRQ self-test delivered: 1
IRQ self-test returned: 1
Exception self-test requested: 1
x86_64 exception self-test: ud2
x86_64 exception
Name: invalid opcode
Vector: 0x0000000000000006
Error code: 0x0000000000000000
x86_64 panic halt
Panic halt mode: cli; hlt
```

## Milestones

1. Add x86_64 build metadata without changing the default i386 build. Done.
2. Add a separate linker script and boot objects for the x86_64 path. Done for the first experimental path.
3. Enter long mode from a 32-bit bootstrap. Started.
4. Bring up a minimal 64-bit C console path. Started.
5. Parse Multiboot2 boot information and memory map. Started; command-line parsing is now present for controlled boot test flags.
6. Add x86_64 GDT/IDT/interrupt handling. Started with maintained GDT/TSS, IST routing, exception diagnostics, guarded IRQ gates, legacy PIC setup, PIT, keyboard, and opt-in IRQ/exception self-tests.
7. Port paging and memory layout to 64-bit addresses. Started with boot-context layout, PMM planning, PMM-backed page tables, CR3 activation, direct-map/higher-half aliases, guarded higher-half probes, runtime entry, and PMM-backed early heap.
8. Bring up x86_64 process and userspace model. Started with an architecture-local process table and scheduler smoke model using heap-backed kernel stacks.
9. Revisit syscalls and userspace after the 64-bit kernel path is stable.

## Guardrails

- Do not copy i386 files and rename them as if that made them 64-bit safe.
- Keep i386 bootable until x86_64 has its own verified boot path.
- Keep architecture-specific code under architecture-specific directories.
- Promote shared abstractions only after both i386 and x86_64 need the same contract.
