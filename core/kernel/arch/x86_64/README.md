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

It can also build and run an experimental Multiboot2 ISO that enters long mode, calls a freestanding x86_64 C entry, reports a required CPU capability baseline, loads a maintained GDT with a 64-bit TSS descriptor, installs an early exception IDT after the TSS is ready, routes NMI through IST2, double fault through IST1, and page fault through IST3, builds an architecture boot context, parses boot information, computes an early PMM plan, initializes an isolated bounded PMM page allocator, captures bootstrap paging state, reports an explicit higher-half/direct-map virtual memory plan, builds C-owned page tables for that plan, switches CR3 to those C-owned tables, probes identity/direct-map/higher-half kernel aliases, prepares bootstrap IST stacks, and writes VGA/serial diagnostics:

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
Stage: paging probes + descriptor
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

If an early CPU exception fires after IDT installation, the handler prints the exception name over VGA and the full vector/error-code diagnostics over serial.

`make x86_64-run` also routes COM1 serial output to the terminal, including CPU capability diagnostics, bootstrap paging diagnostics for the original CR3, planned virtual memory diagnostics for the higher-half kernel and direct physical map, C-owned page-table diagnostics, active CR3 diagnostics after switching to the C-owned tables, identity/direct-map/higher-half alias probe diagnostics, IDTR base/limit, NMI/double-fault/page-fault IST routing, GDTR base/limit, active selectors, current task-register selector, descriptor values, TSS base/limit, and planned IST stack addresses.

The x86_64 path also has a headless smoke target for CI and automated agent testing:

```sh
cd core
make x86_64-smoke
```

That target runs QEMU without a window, captures serial output to `core/build/x86_64/x86_64-smoke.log`, and validates these serial markers:

```txt
Liam_OS x86_64 kernel diagnostics
Stage: paging probes + descriptor
Multiboot2: ok
CPU CPUID available: 1
CPU FPU: 1
CPU MSR: 1
CPU APIC: 1
CPU SSE: 1
CPU SSE2: 1
CPU SYSCALL/SYSRET: 1
CPU NX: 1
CPU long mode: 1
CPU baseline ok: 1
IDT NMI IST: 2
IDT NMI IST ok: 1
IDT double fault IST: 1
IDT double fault IST ok: 1
IDT page fault IST: 3
IDT page fault IST ok: 1
IDT IST gates ok: 1
PMM smoke free: 1
Paging huge pages: 512
VM identity PML4 index: 0
VM kernel PML4 index: 511
VM direct map PML4 index: 256
VM planned regions: 3
VM identity window ok: 1
VM kernel canonical: 1
VM direct map canonical: 1
VM PML4 slots distinct: 1
VM plan ok: 1
Paging builder ok: 1
Paging activation ok: 1
Paging probe activation ready: 1
Paging probe identity ok: 1
Paging probe direct map ok: 1
Paging probe kernel alias ok: 1
Paging probes ok: 1
GDT/TSS loaded ok: 1
Desc/IST ok: 1
```

GitHub Actions runs the same smoke target and uploads the serial log when the workflow completes.

This is not the full x86_64 kernel yet. The current path proves an assembly handoff into long mode, a minimal C entry, early boot diagnostics, Multiboot2 tag parsing, CPU capability baseline reporting, an early CPU exception IDT, dedicated critical-exception IST routing, a C-built maintained GDT, a loaded x86_64 TSS descriptor, an architecture-owned boot context, a planning PMM view over retained memory-map regions, an isolated physical page allocator smoke test, C-visible bootstrap paging-state diagnostics, a C-visible higher-half/direct-map virtual memory plan, a C-owned page-table set for that plan, activation of those page tables through CR3, validation that planned early aliases read the same kernel image bytes, a C-visible bootstrap TSS/IST plan, and automated headless boot validation. It does not jump execution to the higher-half kernel window yet, and it does not initialize IRQ routing, APIC/PIC/PIT, the shared paging subsystem, heap, processes, syscalls, or userspace.

## Milestones

1. Add x86_64 build metadata without changing the default i386 build. Done.
2. Add a separate linker script and boot objects for the x86_64 path. Done for the first experimental path.
3. Enter long mode from a 32-bit bootstrap. Started.
4. Bring up a minimal 64-bit C console path. Started.
5. Parse Multiboot2 boot information and memory map. Started.
6. Add x86_64 GDT/IDT/interrupt handling. Started with a maintained GDT, loaded TSS, critical-exception IST routing, and exception IDT.
7. Port paging and memory layout to 64-bit addresses. Started with boot-context layout, PMM planning diagnostics, an isolated PMM allocator smoke test, bootstrap paging-state diagnostics, a smoke-validated higher-half/direct-map plan, smoke-validated C-owned page tables, CR3 activation of those tables, and active alias probes.
8. Revisit syscalls and userspace after the 64-bit kernel path is stable.

## Guardrails

- Do not copy i386 files and rename them as if that made them 64-bit safe.
- Keep i386 bootable until x86_64 has its own verified boot path.
- Keep architecture-specific code under architecture-specific directories.
- Promote shared abstractions only after both i386 and x86_64 need the same contract.
