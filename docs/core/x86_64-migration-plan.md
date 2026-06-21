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
- Replace temporary bootstrap paging with a planned x86_64 memory layout.

## Stage 4: early 64-bit kernel entry

Status: started.

- Added `core/kernel/arch/x86_64/kernel.c` with a minimal freestanding C entry point.
- Added `core/kernel/arch/x86_64/console.c` and `console.h` for early VGA text and COM1 serial output.
- Added `core/kernel/arch/x86_64/types.h` for local fixed-width types.
- `make x86_64-run` routes serial output to the terminal with `-serial stdio`.

Remaining work:

- Add stronger serial-driven boot validation.
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

Status: started for bootstrap GDT and CPU exceptions.

- Added `core/kernel/arch/x86_64/idt.c` and `idt.h` for early IDT setup.
- Added `core/kernel/arch/x86_64/idt_stubs.asm` for exception vectors 0 through 31.
- Installed interrupt-gate descriptors for CPU exceptions after the C console and serial paths initialize.
- Added VGA and serial exception diagnostics with vector, error code, and exception name.
- Added `core/kernel/arch/x86_64/gdt.c` and `gdt.h` to capture GDTR, active segment selectors, and bootstrap descriptor values from C.
- The x86_64 C entry reports GDT selector validation before a maintained descriptor module and TSS are introduced.

Remaining work:

- Move the bootstrap GDT into a maintained x86_64 descriptor module.
- Add x86_64 TSS planning and IST stacks for dangerous exceptions such as double fault.
- Add IRQ routing deliberately, deciding what remains legacy PIC/PIT and what moves toward APIC later.
- Keep interrupts disabled until the IRQ and timer strategy is explicit.

## Stage 7: 64-bit memory model

Status: started with layout, PMM planning, an isolated allocator smoke test, and bootstrap paging-state diagnostics.

- Added `core/kernel/arch/x86_64/memory_layout.c` and `memory_layout.h`.
- Exposed linker-provided x86_64 kernel image start and end symbols.
- Report current kernel image bounds, kernel image size, bootstrap identity-map span, boot stack size, and page-size constants through the boot context.
- Added `core/kernel/arch/x86_64/pmm_plan.c` and `pmm_plan.h`.
- Compute a planning PMM view from retained Multiboot2 memory regions by reserving everything below the aligned kernel image end.
- Report PMM usable region count, first planned free page, managed page count, managed bytes, and reserved-below boundary.
- Added `core/kernel/arch/x86_64/pmm.c` and `pmm.h` as an isolated bounded page-stack allocator for early x86_64 physical pages.
- The allocator rejects duplicate frees, unaligned pages, invalid pages, and obvious out-of-range frees.
- The x86_64 C entry performs a one-page allocate/free smoke test and reports the result over VGA and serial.
- Added `core/kernel/arch/x86_64/paging.c` and `paging.h` to capture the bootstrap CR3 and identity-mapped huge-page table state.
- The x86_64 C entry reports the current bootstrap paging baseline before a permanent paging model is introduced.

Remaining work:

- Replace the temporary identity map with an architecture-owned PML4 bootstrap builder.
- Introduce pointer-width-safe address types where needed.
- Define the permanent x86_64 virtual memory map separately from i386 constants.
- Connect the x86_64 PMM to page-table allocation only after the bootstrap paging baseline stays stable.
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
