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

It can also build and run an experimental Multiboot2 ISO that enters long mode, calls a freestanding x86_64 C entry, reports a required CPU capability baseline, loads a maintained GDT with a 64-bit TSS descriptor, installs an early exception IDT after the TSS is ready, routes NMI through IST2, double fault through IST1, and page fault through IST3, reports a guarded IRQ policy while keeping hardware interrupts disabled, parses a bounded Multiboot2 command line, reports page-fault CR2/error-code diagnostic readiness, reports exception panic-halt readiness, supports an opt-in invalid-opcode exception self-test, builds an architecture boot context, parses boot information, computes an early PMM plan, initializes an isolated bounded PMM page allocator, captures bootstrap paging state, reports an explicit higher-half/direct-map virtual memory plan, builds C-owned page tables for that plan, switches CR3 to those C-owned tables, probes identity/direct-map/higher-half kernel data aliases, proves assembly and C higher-half execution probes can run through the kernel alias, validates a guarded higher-half handoff call using the current ABI and stack, enters a guarded higher-half runtime entry, prepares bootstrap IST stacks, and writes VGA/serial diagnostics:

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
Stage: higher-half runtime entry
Multiboot2: ok
Bootloader: ...
Boot info pointer: 0x........
Boot info bytes: ...
Memory map entries: ...
Usable bytes: 0x................
CPU baseline ok: 1
IDT IST gates: 1
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
Desc/IST ok: 1
```

If an early CPU exception fires after IDT installation, the handler prints the exception name over VGA and the full vector/error-code diagnostics over serial. Page faults additionally report CR2 plus decoded present/write/user/reserved-bit/instruction-fetch error-code flags. All exceptions terminate through the shared panic halt helper, which announces the `cli; hlt` halt mode before stopping the CPU.

`make x86_64-run` routes COM1 serial output to the terminal, including CPU capability diagnostics, bootstrap paging diagnostics for the original CR3, planned virtual memory diagnostics for the higher-half kernel and direct physical map, C-owned page-table diagnostics, active CR3 diagnostics after switching to the C-owned tables, identity/direct-map/higher-half data alias probe diagnostics, higher-half assembly/C execution and handoff probe diagnostics, runtime-entry diagnostics, IDTR base/limit, NMI/double-fault/page-fault IST routing, page-fault diagnostic readiness, panic-halt readiness, guarded IRQ policy diagnostics, GDTR base/limit, active selectors, current task-register selector, descriptor values, TSS base/limit, and planned IST stack addresses.

This is not the full x86_64 kernel yet. The current path proves an assembly handoff into long mode, a minimal C entry, early boot diagnostics, Multiboot2 tag parsing, bounded command-line parsing, CPU capability baseline reporting, an early CPU exception IDT with page-fault diagnostics and panic-halt readiness prepared, a guarded IRQ policy with interrupts still disabled, an opt-in invalid-opcode exception self-test, dedicated critical-exception IST routing, a C-built maintained GDT, a loaded x86_64 TSS descriptor, an architecture-owned boot context, a planning PMM view over retained memory-map regions, an isolated physical page allocator smoke check, C-visible bootstrap paging-state diagnostics, a C-visible higher-half/direct-map virtual memory plan, a C-owned page-table set for that plan, activation of those page tables through CR3, validation that planned early aliases read the same kernel image bytes, safe higher-half assembly/C probes through the kernel alias, a guarded higher-half handoff probe through the kernel alias, a guarded higher-half runtime entry, and a C-visible bootstrap TSS/IST plan. It does not remove the low identity transition map yet, and it does not enable IRQ delivery, initialize APIC/PIC/PIT, initialize the shared paging subsystem, heap, processes, syscalls, or userspace.

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
IDT PF CR2 reporting: 1
IDT PF error decode: 1
IDT panic halt ready: 1
IDT panic cli before hlt: 1
IDT diagnostics ok: 1
IRQ interrupts enabled: 0
IRQ interrupts guarded: 1
IRQ legacy vector base: 32
IRQ legacy vector count: 16
IRQ PIT planned vector: 32
IRQ keyboard planned vector: 33
IRQ APIC deferred: 1
IRQ policy ok: 1
Runtime high entry ready: 1
Runtime arg ok: 1
Runtime CR3 ok: 1
Runtime return ok: 1
Runtime entered ok: 1
Runtime scratch ok: 1
Runtime stack identity ok: 1
Runtime entry ok: 1
Higher-half handoff ok: 1
Higher-half C probe ok: 1
Paging probes ok: 1
Paging activation ok: 1
VM plan ok: 1
Desc/IST ok: 1
```

Use this separate opt-in path for the x86_64 exception self-test:

```sh
cd core
make clean
make x86_64-exception-test-iso
make x86_64-exception-test-run
```

Check the serial output for:

```txt
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
6. Add x86_64 GDT/IDT/interrupt handling. Started with a maintained GDT, loaded TSS, critical-exception IST routing, page-fault diagnostics, panic-halt readiness, guarded IRQ policy diagnostics, an opt-in invalid-opcode exception self-test, and exception IDT.
7. Port paging and memory layout to 64-bit addresses. Started with boot-context layout, PMM planning diagnostics, an isolated PMM allocator smoke test, bootstrap paging-state diagnostics, a smoke-validated higher-half/direct-map plan, smoke-validated C-owned page tables, CR3 activation of those tables, active alias probes, safe higher-half assembly/C execution probes, a guarded higher-half handoff probe, and a guarded higher-half runtime entry.
8. Revisit syscalls and userspace after the 64-bit kernel path is stable.

## Guardrails

- Do not copy i386 files and rename them as if that made them 64-bit safe.
- Keep i386 bootable until x86_64 has its own verified boot path.
- Keep architecture-specific code under architecture-specific directories.
- Promote shared abstractions only after both i386 and x86_64 need the same contract.
