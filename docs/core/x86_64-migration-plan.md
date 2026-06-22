# Liam_OS x86_64 Migration Plan

Liam_OS must move toward a real x86_64 kernel path without breaking the current i386 bootable baseline. This plan keeps the migration staged, reviewable, and honest about what is and is not bootable at each step.

## Current baseline

- Default build remains `ARCH=i386`.
- The known-good boot path remains GRUB Multiboot into a 32-bit protected-mode kernel.
- Current userspace remains 32-bit flat binaries plus limited ELF32 support.
- Current syscall ABI remains `int 0x80` with 32-bit register arguments.

## Stage 1: architecture scaffolding

Status: complete for the initial scaffold.

- Keep `core/kernel/arch/i386/` untouched as the bootable path.
- Add `core/kernel/arch/x86_64/` as a real architecture directory, initially documentation-only.
- Add Makefile architecture metadata for `i386` and `x86_64`.
- Keep normal `make`, `make check-tools`, and `make run` i386-only.
- Add an informational target for the x86_64 plan.
- Refuse accidental `ARCH=x86_64` builds until a real bootstrap exists.

## Stage 2: x86_64 boot objects

Status: complete for the first experimental objects.

- Added `core/linker.x86_64.ld` for a 64-bit kernel image layout.
- Added `core/kernel/arch/x86_64/start.asm` as the first x86_64 entry object.
- Added `make x86_64-kernel` to build `core/build/x86_64/kernel.elf` without changing the i386 ISO path.
- Added `core/kernel/arch/x86_64/boot32.asm` as a Multiboot2 32-bit handoff that creates temporary identity-mapped long-mode paging.
- Added `core/boot/grub-x86_64.cfg` for the experimental x86_64 ISO path.

## Stage 3: long mode transition

Status: started.

- Added `make x86_64-iso` to create `core/build/x86_64/liam_os_x86_64.iso`.
- Added `make x86_64-run` to boot the experimental ISO in `qemu-system-x86_64`.
- The handoff enters long mode, sets 64-bit segments, installs a temporary stack, and calls the x86_64 entry shim.
- The handoff preserves GRUB's Multiboot2 magic and boot information pointer for C.
- This path is still experimental and uses temporary identity-mapped paging.
- The bootstrap assembly now uses BSS-safe alignment and explicit 64-bit boot-state reads to keep assembly diagnostics clean.
- The bootstrap PML4, PDPT, and PD tables now have named symbols that C diagnostics can inspect.
- The bootstrap GDT entries now have named symbols that C diagnostics can inspect.

Remaining work:

- Add explicit failure diagnostics for unsupported CPUs or malformed boot state before enabling long mode.
- Move the full C runtime to the planned higher-half kernel window after the guarded higher-half handoff probe is stable.

## Stage 4: early 64-bit kernel entry

Status: started.

- Added `core/kernel/arch/x86_64/kernel.c` with a minimal freestanding C entry point.
- Added `core/kernel/arch/x86_64/console.c` and `console.h` for early VGA text and COM1 serial output.
- Added `core/kernel/arch/x86_64/types.h` for local fixed-width types.
- Added `core/kernel/arch/x86_64/cpuid.c` and `cpuid.h` for early CPU baseline capability diagnostics.
- `make x86_64-run` routes serial output to the terminal with `-serial stdio`.

Remaining work:

- Expand serial-driven boot validation as new subsystems come online.
- Move any mandatory pre-long-mode CPU gate into the bootstrap once the C-side baseline stays stable.
- Avoid porting scheduler, userspace, or syscalls until basic boot and diagnostics are stable.

## Stage 5: Multiboot2 and memory discovery

Status: started.

- Added `core/kernel/arch/x86_64/boot_info.c` and `boot_info.h`.
- Parse Multiboot2 total size, bootloader name, basic memory info, and memory map tags.
- Report bootloader name, boot-info size, memory-map entry count, and total usable memory bytes over VGA and serial.
- Added `core/kernel/arch/x86_64/boot_context.c` and `boot_context.h` to preserve parsed boot information inside an architecture-owned boot context.
- Retain a bounded copy of Multiboot2 memory-map regions for later architecture initialization.

Remaining work:

- Keep the boot context alive as the handoff into PMM, paging, and later kernel runtime initialization grows.
- Expand memory-map validation before relying on the allocator for core kernel subsystems.

## Stage 6: descriptor and interrupt strategy

Status: started for maintained GDT, loaded TSS, critical-exception IST routing, CPU capability diagnostics, and CPU exceptions.

- Added `core/kernel/arch/x86_64/idt.c` and `idt.h` for early IDT setup.
- Added `core/kernel/arch/x86_64/idt_stubs.asm` for exception vectors 0 through 31.
- Installed interrupt-gate descriptors for CPU exceptions after the maintained GDT/TSS path initializes.
- Added VGA and serial exception diagnostics with vector, error code, and exception name.
- Added `core/kernel/arch/x86_64/gdt.c` and `gdt.h` to install a maintained x86_64 GDT from C.
- Added `core/kernel/arch/x86_64/tss.c` and `tss.h` to build a packed 64-bit TSS image and planned IST stacks for dangerous exceptions.
- Added a 64-bit TSS descriptor to the maintained GDT and load it with `ltr`.
- Routed the double-fault exception gate through IST1.
- Routed the NMI exception gate through IST2.
- Routed the page-fault exception gate through IST3.
- Report required CPU baseline capabilities from C, including CPUID, FPU, MSR, APIC, SSE, SSE2, SYSCALL/SYSRET, NX, and long mode.
- The x86_64 C entry reports IDTR state, critical-exception IST routing, GDT selector validation, current task-register selector, and TSS load validation.

Remaining work:

- Add safe exception-path tests only after the panic/reporting path is ready for intentional faults.
- Add IRQ routing deliberately, deciding what remains legacy PIC/PIT and what moves toward APIC later.
- Keep interrupts disabled until the IRQ and timer strategy is explicit.

## Stage 7: 64-bit memory model

Status: started with layout, PMM planning, an isolated allocator smoke check, bootstrap paging-state diagnostics, a higher-half/direct-map plan, C-owned page tables, CR3 activation of those tables, active alias probes, safe higher-half assembly/C execution probes, and a guarded higher-half handoff probe.

- Added `core/kernel/arch/x86_64/memory_layout.c` and `memory_layout.h`.
- Exposed linker-provided x86_64 kernel image start and end symbols.
- Report current kernel image bounds, kernel image size, bootstrap identity-map span, boot stack size, and page-size constants through the boot context.
- Added `core/kernel/arch/x86_64/pmm_plan.c` and `pmm_plan.h`.
- Compute a planning PMM view from retained Multiboot2 memory regions by reserving everything below the aligned kernel image end.
- Report PMM usable region count, first planned free page, managed page count, managed bytes, and reserved-below boundary.
- Added `core/kernel/arch/x86_64/pmm.c` and `pmm.h` as an isolated bounded page-stack allocator for early x86_64 physical pages.
- The allocator rejects duplicate frees, unaligned pages, invalid pages, and obvious out-of-range frees.
- The x86_64 C entry performs a one-page allocate/free smoke check and reports the result over VGA and serial.
- Added `core/kernel/arch/x86_64/paging.c` and `paging.h` to capture the bootstrap CR3 and identity-mapped huge-page table state.
- The x86_64 C entry reports the current bootstrap paging baseline before a permanent paging model is introduced.
- Added `core/kernel/arch/x86_64/paging_plan.c` and `paging_plan.h` to define the planned higher-half kernel window, direct physical map window, and transition identity-map window.
- The x86_64 diagnostics report planned PML4 slots, canonical virtual address windows, planned-region count, and `VM plan ok: 1`.
- Added `core/kernel/arch/x86_64/paging_builder.c` and `paging_builder.h` to construct C-owned page tables for the planned identity, direct-map, and higher-half kernel windows.
- The x86_64 diagnostics report table alignment, PML4 population, identity/direct huge-page coverage, the 4 KiB kernel mapping, and `Paging builder ok: 1`.
- The x86_64 C entry switches CR3 to the C-built page tables after the builder validates them.
- The x86_64 diagnostics report `Paging activation builder ready: 1`, `Paging activation active matches builder: 1`, and `Paging activation ok: 1`.
- The x86_64 C entry probes the kernel image through the identity map, direct physical map, and higher-half kernel alias after the CR3 switch.
- The x86_64 diagnostics report `Paging probe identity ok: 1`, `Paging probe direct map ok: 1`, `Paging probe kernel alias ok: 1`, and `Paging probes ok: 1`.
- Added a tiny assembly-only higher-half execution probe that returns a fixed value without touching globals or stack-owned data beyond the call/return path.
- The x86_64 diagnostics report `Higher-half probe low ok: 1`, `Higher-half probe high ok: 1`, and `Higher-half probe ok: 1`.
- Added a C higher-half execution probe that reads a kernel marker through normal compiler-generated code/data access.
- The x86_64 diagnostics report `Higher-half C probe low ok: 1`, `Higher-half C probe high ok: 1`, and `Higher-half C probe ok: 1`.
- Added a guarded higher-half handoff probe that calls a C function through the higher-half kernel alias, passes an argument, uses the active stack/calling convention, writes a scratch result, and returns a validation marker.
- The x86_64 diagnostics report `Higher-half handoff ready: 1`, `Higher-half handoff result ok: 1`, `Higher-half handoff scratch ok: 1`, and `Higher-half handoff ok: 1`.

Remaining work:

- Relocate the real C runtime path to the higher-half mapping after the guarded handoff probe remains stable.
- Introduce pointer-width-safe address types where needed.
- Connect the x86_64 PMM to page-table allocation only after the active paging baseline stays stable.
- Keep PMM/VMM interfaces honest about physical and virtual address width.

## Stage 8: process and userspace model

- Keep 32-bit userspace out of the first x86_64 kernel boot milestone.
- Decide whether early x86_64 userspace starts as 64-bit flat binaries, ELF64, or compatibility-mode experiments.
- Design a syscall ABI separately from the current i386 `int 0x80` path.
- Add process address spaces before treating userspace as isolated.

## Non-goals for early migration

- Do not rename i386 files to x86_64.
- Do not port GUI, desktop, installer, or product UX work into the kernel migration branch.
- Do not treat the x86_64 path as complete until it has a real C kernel entry, interrupts, paging model, memory layout, and userspace strategy.
