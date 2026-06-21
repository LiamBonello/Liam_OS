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

Remaining work:

- Preserve parsed boot information in a durable architecture boot context.
- Convert memory-map data into an x86_64 physical memory initialization contract.
- Define the x86_64 kernel virtual memory layout before enabling higher-half mappings.

## Stage 6: descriptor and interrupt strategy

- Add x86_64 GDT support with long-mode code/data descriptors and TSS planning.
- Add x86_64 IDT entries and interrupt stubs.
- Revisit PIC/PIT handling and decide what stays legacy versus what moves toward APIC later.

## Stage 7: 64-bit memory model

- Replace 32-bit page-directory/page-table assumptions with PML4-based paging for x86_64.
- Introduce pointer-width-safe address types where needed.
- Define an x86_64 memory layout separately from i386 constants.
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
