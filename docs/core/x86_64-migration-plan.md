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
- Add `core/kernel/arch/x86_64/` as a real architecture directory.
- Add Makefile architecture metadata for `i386` and `x86_64`.
- Keep normal `make`, `make check-tools`, and `make run` i386-only.
- Add staged x86_64 targets instead of silently changing the default build.

## Stage 2: x86_64 boot objects

Status: complete for the first experimental objects.

- Added `core/linker.x86_64.ld` for a 64-bit kernel image layout.
- Added x86_64 bootstrap and entry objects under `core/kernel/arch/x86_64/`.
- Added Multiboot2 GRUB configs for normal and opt-in exception-test x86_64 ISO paths.

## Stage 3: long mode transition

Status: started.

- `make x86_64-iso` creates `core/build/x86_64/liam_os_x86_64.iso`.
- `make x86_64-run` boots the experimental ISO in `qemu-system-x86_64`.
- The bootstrap enters long mode, sets 64-bit segments, installs a temporary stack, preserves GRUB Multiboot2 state, and calls the x86_64 C entry.

Remaining work:

- Add explicit failure diagnostics for unsupported CPUs or malformed boot state before enabling long mode.
- Remove the low identity transition dependency after the higher-half runtime entry remains stable.

## Stage 4: early 64-bit kernel entry

Status: started.

- Added early VGA/COM1 console output, local fixed-width types, CPU capability diagnostics, and a guarded higher-half runtime entry boundary.
- x86_64 run targets route serial output to the terminal for validation.

Remaining work:

- Expand serial-driven boot validation as new subsystems come online.
- Move mandatory CPU gates into bootstrap once the C-side baseline stays stable.

## Stage 5: Multiboot2 and memory discovery

Status: started.

- Parse Multiboot2 total size, bounded command line, bootloader name, basic memory info, and memory map tags.
- Retain a bounded memory-map copy inside an architecture-owned boot context.
- Recognize controlled test flags for exception and IRQ self-tests.

Remaining work:

- Keep the boot context alive as PMM, paging, runtime, and userspace initialization grow.
- Expand memory-map validation before relying on the allocator for long-lived subsystems.

## Stage 6: descriptor and interrupt strategy

Status: started.

- Maintained x86_64 GDT and loaded TSS descriptor are active.
- NMI, double fault, and page fault use dedicated IST slots.
- CPU exception handlers report vector/error-code diagnostics and panic through a shared `cli; hlt` halt path.
- Legacy PIC is remapped, IRQ gates are installed, and opt-in IRQ/exception self-tests validate gate delivery and fault reporting.
- Normal x86_64 boot now runs bounded PIT and keyboard IRQ diagnostics.

Remaining work:

- Decide what remains legacy PIC/PIT and what moves toward APIC later.
- Grow IRQ dispatch from diagnostics into normal kernel interrupt services.

## Stage 7: 64-bit memory model

Status: started.

- Added boot-context memory layout reporting, PMM planning, and an isolated bounded PMM allocator.
- Added a higher-half/direct-map virtual memory plan.
- Built PMM-backed C-owned page tables and switched CR3 to them.
- Validated identity, direct-map, and higher-half aliases plus guarded higher-half execution/runtime-entry probes.
- Added a PMM-backed early heap over the direct-map window with rollback on partial allocation failure.

Remaining work:

- Remove the low identity-map dependency from the permanent runtime path after the guarded runtime entry remains stable.
- Introduce pointer-width-safe address types where needed.
- Replace the early heap with a fuller x86_64 kernel heap/VMM layer when process address spaces arrive.

## Stage 8: process and userspace model

Status: started.

- Added an x86_64 architecture-local process table and scheduler smoke model.
- Current process states are READY, RUNNING, EXITED, FAILED, and UNUSED.
- Early process records allocate kernel stacks from the PMM-backed early heap.
- The normal x86_64 boot creates two kernel smoke processes, runs all ready processes, and validates creation/run/exit counters.

Remaining work:

- Add real x86_64 context switching instead of direct function calls.
- Add per-process address spaces and VMM ownership.
- Design the x86_64 syscall ABI separately from the i386 `int 0x80` path.
- Add ELF64 loading and an isolated x86_64 userspace shell only after kernel scheduling and address spaces are stable.

## Non-goals for early migration

- Do not rename i386 files to x86_64.
- Do not remove i386 until x86_64 can do what the i386 path currently does and a final comparison pass confirms parity.
- Do not port GUI, desktop, installer, or product UX work into the kernel migration branch.
- Do not treat the x86_64 path as complete until it has a real C kernel entry, interrupts, paging model, memory layout, scheduler, syscall ABI, and userspace strategy.
