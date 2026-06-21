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

Status: started with a non-bootable ELF64 artifact.

- Added `core/linker.x86_64.ld` for a 64-bit kernel image layout.
- Added `core/kernel/arch/x86_64/start.asm` as the first x86_64 entry object.
- Added `make x86_64-kernel` to build `core/build/x86_64/kernel.elf` without changing the i386 ISO path.
- The artifact is an ELF64 kernel experiment only. It assumes 64-bit execution already exists and is not yet a real boot path.

Remaining work:

- Decide whether the first x86_64 boot path uses Multiboot2 or a transitional Multiboot handoff.
- Build only the minimal boot objects needed for a controlled long-mode entry experiment.
- Keep the i386 ISO path unchanged.

## Stage 3: long mode transition

- Build temporary 32-bit bootstrap paging structures for long mode.
- Enable PAE, load PML4, enable EFER.LME, enable paging, and far-jump into 64-bit code.
- Establish a known-good 64-bit stack.
- Halt cleanly on failure paths instead of falling into undefined execution.

## Stage 4: early 64-bit kernel entry

- Add a minimal `kernel_main_x86_64` or architecture entry contract.
- Bring up enough VGA/serial logging to prove 64-bit execution.
- Avoid porting scheduler, userspace, or syscalls until basic boot and diagnostics are stable.

## Stage 5: descriptor and interrupt strategy

- Add x86_64 GDT support with long-mode code/data descriptors and TSS planning.
- Add x86_64 IDT entries and interrupt stubs.
- Revisit PIC/PIT handling and decide what stays legacy versus what moves toward APIC later.

## Stage 6: 64-bit memory model

- Replace 32-bit page-directory/page-table assumptions with PML4-based paging for x86_64.
- Introduce pointer-width-safe address types where needed.
- Define an x86_64 memory layout separately from i386 constants.
- Keep PMM/VMM interfaces honest about physical and virtual address width.

## Stage 7: process and userspace model

- Keep 32-bit userspace out of the first x86_64 kernel boot milestone.
- Decide whether early x86_64 userspace starts as 64-bit flat binaries, ELF64, or compatibility-mode experiments.
- Design a syscall ABI separately from the current i386 `int 0x80` path.
- Add process address spaces before treating userspace as isolated.

## Non-goals for early migration

- Do not rename i386 files to x86_64.
- Do not port GUI, desktop, installer, or product UX work into the kernel migration branch.
- Do not claim x86_64 bootability until QEMU reaches a controlled 64-bit kernel message or halt path.
