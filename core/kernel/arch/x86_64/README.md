# Liam_OS x86_64 Architecture Path

This directory contains the staged x86_64 kernel path. The i386 path in `core/kernel/arch/i386/` remains the stable default while x86_64 grows to feature parity in coherent, testable steps.

## Current status

The x86_64 path builds an experimental ELF64 kernel artifact through:

```sh
cd core
make x86_64-kernel
```

The artifact is written to:

```txt
core/build/x86_64/kernel.elf
```

It can also build and run an experimental Multiboot2 ISO that enters long mode, calls a freestanding x86_64 C entry, reports CPU capability diagnostics, loads a maintained GDT/TSS, installs exception and IRQ gates, remaps the legacy PIC, runs bounded PIT and keyboard diagnostics, parses Multiboot2 boot data, initializes an architecture-owned boot context, brings up an isolated PMM, builds PMM-backed page tables, switches CR3 to the C-owned tables, validates identity/direct-map/higher-half aliases, initializes a direct-map early heap, programs the fast-syscall MSRs, enters ring 3, dispatches live user syscalls, loads an embedded ELF64 init image, and boots a minimal x86_64 user shell.

```sh
cd core
make x86_64-iso
make x86_64-run
```

The ISO is written to:

```txt
core/build/x86_64/liam_os_x86_64.iso
```

`make x86_64-run` is terminal-interactive. It runs QEMU with serial input and output attached to the terminal, so type shell commands in the same terminal that launched QEMU. The older graphical path remains available through:

```sh
make x86_64-run-gui
```

## Expected normal boot markers

The normal x86_64 run should report key serial markers including:

```txt
Syscall ABI ok: 1
Syscall MSR ok: 1
User mode ok: 1
IRQ gates ok: 1
PIT timer ok: 1
Keyboard ready ok: 1
Paging builder ok: 1
Paging activation ok: 1
Paging probes ok: 1
Runtime entry ok: 1
Heap smoke ok: 1
Desc/IST ok: 1
Liam_OS x86_64 shell online
```

The interactive shell currently supports:

```txt
help
about
version
pid
echo <text>
clear
exit
```

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

At the shell prompt, test:

```txt
help
about
version
pid
echo hello from x86_64
clear
exit
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

## Remaining milestones before x86_64 can become default

1. Replace the smoke scheduler with real x86_64 context switching.
2. Add process-owned address spaces instead of the current shared early mapping.
3. Grow the syscall table from guarded basics into real kernel services.
4. Add filesystem-backed ELF64 program loading instead of only the embedded init image.
5. Build out the shell and userland commands against real `open/read/stat/exec` services.
6. Run an i386-vs-x86_64 parity pass before changing the default build.
7. Remove i386 only after x86_64 can do what the current i386 path does.

## Guardrails

- Do not copy i386 files and rename them as if that made them 64-bit safe.
- Keep i386 bootable until x86_64 has verified parity.
- Keep architecture-specific code under architecture-specific directories.