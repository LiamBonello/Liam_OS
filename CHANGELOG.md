# Liam_OS Changelog

## Core 0.8.66-dev

### Added

- Added an x86_64 desktop-services kernel layer that reports timer-backed scheduler readiness, keyboard input readiness, read-only VFS storage capability, framebuffer graphics readiness, and window-service readiness.
- Added `desktop_status` and `window_present` syscall services for userspace desktop plumbing.
- Added `/bin/windowd`, a first userspace window service probe that reads desktop status and asks the kernel to draw a demo window.
- Extended the x86_64 smoke check to run `/bin/windowd`, wait for it, and verify desktop service markers.

### Changed

- Updated the x86_64 `/proc/version` text and Liam_OS version to `0.8.66-dev`.
- Updated `/bin/sysinfo` feature text and VFS help/listing output for the desktop-services milestone.

### Notes

- Persistent storage is still reported as not ready. This milestone exposes the current read-only VFS capability honestly while creating the kernel/userland contract that a future storage-backed filesystem can plug into.

## Core 0.8.65-dev

### Added

- Added an x86_64 framebuffer drawing surface on top of the higher-half boot framebuffer mapping.
- Added RGB color packing, pixel writes, rectangle fills, and a framebuffer smoke draw/readback check.
- Extended the x86_64 QEMU smoke check to verify that the framebuffer surface initializes and can draw.

### Changed

- Updated the x86_64 `/proc/version` text and Liam_OS version to `0.8.65-dev`.

### Notes

- The kernel now has the first usable graphics primitive layer for desktop work. The next desktop-facing pieces are input event plumbing, timer-driven scheduling for long-running services, and a userspace graphics/window service.

## Core 0.8.64-dev

### Added

- Added an x86_64 Multiboot2 framebuffer request for a 1024x768x32 RGB mode.
- Parsed and reported Multiboot2 framebuffer metadata, including address, pitch, dimensions, bpp, type, and RGB channel layout.
- Added a fixed higher-half framebuffer virtual mapping at `0xFFFF900000000000` backed by huge pages.
- Extended the x86_64 QEMU smoke check to verify framebuffer discovery, RGB format readiness, virtual mapping readiness, and shell/process markers.

### Changed

- Made the x86_64 smoke check perform a clean build so header changes cannot be hidden by stale objects.
- Updated the x86_64 `/proc/version` text and Liam_OS version to `0.8.64-dev`.

### Notes

- This is the first kernel-side desktop graphics foundation. It discovers and maps the boot framebuffer, but it does not yet provide a compositor, drawing API, GUI input stack, or userspace graphics service.

## Core 0.8.63-dev

### Added

- Replaced the single x86_64 child wait slot with a bounded FIFO child-status queue.
- Added queue-depth, high-watermark, and dropped-status diagnostics to the x86_64 process manager and `ps` output.
- Extended the process smoke path to record and wait on two child statuses in FIFO order.

### Changed

- `wait` can now consume multiple completed child statuses one at a time instead of losing earlier child exits when another child finishes first.
- Updated Liam_OS version to `0.8.63-dev`.

### Notes

- The queue is still intentionally bounded and kernel-owned. If it fills, the oldest child status is dropped and counted, which keeps the shell responsive while exposing pressure through diagnostics.

## Core 0.8.62-dev

### Added

- Added an x86_64 `wait` syscall that lets the shell consume a completed child PID and exit code.
- Added an x86_64 shell `wait` command that reports the last child completion or `wait: no child`.
- Added process diagnostics for recorded child completions, wait calls, wait misses, and last wait status.
- Added syscall dispatcher smoke checks for empty wait and invalid wait pointer handling.

### Changed

- Recorded child completion status before user process reaping so exit state survives long enough for the parent shell to observe it.
- Updated x86_64 shell help text and README command examples to include `wait`.
- Updated Liam_OS version to `0.8.62-dev`.

### Notes

- This is the first explicit parent-child wait contract. A later scheduler update can add blocking wait once multiple user processes can stay alive at the same time.

## Core 0.8.61-dev

### Added

- Added an x86_64 `ps` syscall that safely formats the kernel process table into a user-provided buffer.
- Added an x86_64 shell `ps` command that reports PID, state, mode, name/path, and process lifecycle counters.
- Added syscall diagnostics for the new process snapshot service and its user-pointer fault path.

### Changed

- Updated x86_64 shell help text and README command examples to include `ps`.
- Updated Liam_OS version to `0.8.61-dev`.

### Notes

- The `ps` output is a read-only kernel snapshot. It does not expose mutable process handles or scheduling controls.
- This builds on the process reaping step by making process lifecycle visible from inside the running x86_64 shell.

## Core 0.8.58-dev

### Added

- Added a structured x86_64 syscall frame for live ring-3 `syscall` entry.
- Routed the live x86_64 ring-3 smoke syscalls through the architecture-local syscall dispatcher instead of a smoke-only syscall switch.
- Added user-mode diagnostics for live dispatcher initialization, dispatcher acceptance, saved syscall frame state, user RIP, and user RFLAGS.

### Changed

- Updated the x86_64 syscall entry stub to save user return state, collect the six-argument syscall ABI registers, call the dispatcher-backed C entrypoint, and return through `sysret` unless `exit` requested kernel return.
- Tightened the x86_64 syscall ABI report so user entry is no longer marked deferred.
- Updated Liam_OS version to `0.8.58-dev`.

### Notes

- This is still a guarded early path, but ring-3 syscalls now use the same dispatcher contract as the kernel-side syscall dispatch smoke checks.
- The normal x86_64 boot should report `Syscall user entry deferred: 0`, `User mode live dispatcher initialized: 1`, `User mode live dispatcher ok: 1`, `User mode syscall frame ok: 1`, and `User mode ok: 1`.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.57-dev

### Added

- Added a guarded x86_64 ring-3 smoke runner that enters user mode through an `iretq` frame and executes the mapped ELF64 smoke image.
- Added a live x86_64 syscall smoke path for `getpid`, `yield`, and `exit`, returning through `sysretq` for continuing calls and restoring the kernel stack on user exit.
- Added serial diagnostics for user-mode entry readiness, stack readiness, syscall count, syscall results, exit code, and overall `User mode ok` status.

### Changed

- Wired the x86_64 boot path to run the ring-3 smoke after the higher-half runtime and before the remaining diagnostics.
- Updated `make x86_64-info` validation markers for the first live ring-3 syscall milestone.
- Updated Liam_OS version to `0.8.57-dev`.

### Notes

- This is the first real x86_64 ring-3 execution checkpoint, but the syscall entry is still a guarded smoke path rather than the final production syscall dispatcher.
- The normal x86_64 boot should report `x86_64 ring3 user smoke online`, `User mode attempted: 1`, `User mode returned to kernel: 1`, `User mode syscall count: 3`, `User mode getpid ok: 1`, `User mode yield ok: 1`, `User mode exit ok: 1`, and `User mode ok: 1`.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.56-dev

### Added

- Added guarded x86_64 syscall MSR bring-up for `IA32_EFER`, `IA32_STAR`, `IA32_LSTAR`, and `IA32_FMASK`.
- Added serial diagnostics that read back the programmed fast-syscall MSRs and report EFER.SCE, STAR, LSTAR, FMASK, and overall `Syscall MSR ok` status.

### Changed

- Tightened the x86_64 syscall ABI report so syscall MSR programming is no longer marked deferred.
- Kept the current LSTAR target on the guarded syscall entry stub until the real ring-3 syscall return path is implemented.
- Updated Liam_OS version to `0.8.56-dev`.

### Notes

- This configures the CPU fast-syscall registers, but it still does not enter x86_64 user mode or dispatch live user syscalls from ring 3.
- The normal x86_64 boot should report `Syscall MSR programming deferred: 0`, `Syscall EFER SCE enabled: 1`, `Syscall STAR programmed: 1`, `Syscall LSTAR programmed: 1`, `Syscall FMASK programmed: 1`, and `Syscall MSR ok: 1`.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.55-dev

### Changed

- Bound the x86_64 process/user-context smoke path to the currently active CR3 instead of using a kernel stack address as a placeholder page-table value.
- Added user-context diagnostics for CR3 page alignment and address-space readiness.
- Tightened `User context ok: 1` so it now requires a nonzero, page-aligned planned CR3.
- Updated Liam_OS version to `0.8.55-dev`.

### Notes

- This is still a kernel-side user context readiness checkpoint. It does not yet switch to ring 3, execute ELF64 user code, or boot the x86_64 shell.
- The normal x86_64 boot should report `User planned CR3: 0x...`, `User CR3 page aligned: 1`, `User address space ready: 1`, `User context ok: 1`, and `Process user context ok: 1`.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.47-dev

### Added

- Added a guarded x86_64 syscall entry stub symbol for the future `IA32_LSTAR` target.
- Added serial diagnostics for syscall entry-stub availability, planned LSTAR target, deferred STAR selector programming, and planned FMASK value.

### Changed

- Tightened the syscall ABI diagnostics so `Syscall entry ready: 1` now requires a concrete guarded entry stub while STAR selector programming remains deferred until x86_64 user GDT entries exist.
- Updated `make x86_64-info` validation markers for the syscall entry checkpoint.
- Updated Liam_OS version to `0.8.47-dev`.

### Notes

- This still does not enable user-mode `syscall/sysret`, program syscall MSRs, or enter x86_64 userspace. The stub is intentionally inert and halts if reached before the full user transition path exists.
- The normal x86_64 boot should report `Syscall entry stub installed: 1`, `Syscall STAR selectors deferred: 1`, `Syscall entry ready: 1`, and `Syscall ABI ok: 1`.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.46-dev

### Added

- Added an x86_64 syscall ABI planning contract for the future `syscall/sysret` path.
- Added serial diagnostics for the planned syscall number, argument, return, clobber, MSR, and user-pointer validation rules.

### Changed

- Wired the syscall ABI report to the x86_64 CPU capability path so the boot log confirms fast syscall support before later ring-3 syscall work.
- Updated Liam_OS version to `0.8.46-dev`.

### Notes

- This does not enable `syscall/sysret`, program syscall MSRs, or enter x86_64 user mode yet. It locks down the ABI contract first so the real syscall entry can be added in a controlled step.
- The normal x86_64 boot should report `x86_64 syscall ABI plan online`, `Syscall fast supported: 1`, `Syscall pointer validation required: 1`, `Syscall entry ready: 0`, and `Syscall ABI ok: 1`.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.44-dev

### Added

- Added an x86_64 architecture-local process table and scheduler smoke model with READY, RUNNING, EXITED, and FAILED process states.
- Added PMM/heap-backed kernel stack allocation for early x86_64 process records.
- Added serial diagnostics for process creation, ready count, run attempts, exited count, stack allocation/alignment, worker execution counters, and overall process smoke status.

### Changed

- Wired the process scheduler smoke check to run after the early x86_64 heap smoke passes, without touching the stable i386 scheduler path.
- Updated `make x86_64-info` validation markers for the process scheduler milestone.
- Updated Liam_OS version to `0.8.44-dev`.

### Notes

- This is a scheduler/process-model stepping stone only. It proves x86_64 process table transitions and heap-backed kernel stacks; it does not yet perform real x86_64 context switching, syscalls, ELF64 loading, isolated address spaces, or a shell.
- The normal x86_64 boot should report `Process created: 2`, `Process ready before run: 2`, `Process run count: 2`, `Process exited: 2`, `Process failed: 0`, `Process stack allocations: 2`, `Process worker A count: 1`, `Process worker B count: 1`, and `Process smoke ok: 1`.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.43-dev

### Added

- Added an x86_64 architecture-local early heap backed by PMM pages and accessed through the direct-map window.
- Added heap allocation smoke diagnostics for PMM backing, direct-map accessibility, allocation count, alignment, pattern writes, and overall heap smoke status.
- Added rollback for partially allocated x86_64 early-heap backing pages if PMM page allocation fails during heap initialization.

### Changed

- Wired the early heap into the x86_64 boot path after PMM-backed page tables are active.
- Updated `make x86_64-info` and README validation markers for the early heap milestone.
- Updated Liam_OS version to `0.8.43-dev`.

### Notes

- This is an early kernel heap/VMM stepping stone for x86_64. It is not yet the shared kernel heap, scheduler heap use, process address-space VMM, syscall ABI, ELF64 loader, or x86_64 shell.
- The normal x86_64 boot should report `Heap PMM backed: 1`, `Heap direct map ok: 1`, `Heap alignment ok: 1`, `Heap pattern ok: 1`, and `Heap smoke ok: 1`.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.42-dev

### Added

- Moved the x86_64 C-owned page-table builder onto the early PMM allocator instead of static bootstrap arrays.
- Added serial diagnostics for `Paging builder PMM backed`, `Paging builder table pages`, `Paging builder PMM free before`, `Paging builder PMM free after`, and `Paging builder allocation ok`.
- Added rollback for partially allocated x86_64 paging-builder tables if a PMM page allocation fails during table construction.

### Changed

- Reordered x86_64 early boot so the PMM allocator is initialized before the C-owned page-table builder needs physical pages.
- Tightened `Paging builder ok: 1` so it requires PMM-backed table allocation and successful allocation accounting.
- Updated Liam_OS version to `0.8.42-dev`.

### Notes

- This is a kernel-memory milestone toward a real x86_64 VMM/heap path; it does not port scheduler/process, syscalls, ELF64 loading, or x86_64 userspace yet.
- The normal x86_64 boot should now show `Paging builder table pages: 8` and a PMM free-page count drop of 8 across the builder allocation.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.41-dev

### Added

- Added an early x86_64 keyboard IRQ diagnostic path on normal x86_64 boots.
- Unmasked legacy IRQ1 after the PIT timer path is live and interrupts are enabled.
- Added PS/2 Set 1 scancode tracking for make codes, break codes, shift state, last scancode, and last translated ASCII value.
- Added serial markers for `x86_64 keyboard IRQ online`, `Keyboard IRQ1 unmasked: 1`, and `Keyboard ready ok: 1`.
- Added per-keypress serial diagnostics for translated keypresses: `x86_64 keyboard IRQ received`, `Keyboard scancode: 0x...`, and `Keyboard ascii: ...`.

### Changed

- Kept x86_64 keyboard input architecture-local until the x86_64 shell and userland path exist.
- Updated Liam_OS version to `0.8.41-dev`.

### Notes

- This proves keyboard IRQ delivery and basic scancode translation, not full x86_64 shell input yet.
- Heap/VMM, PMM-backed page-table allocation, scheduler/process, syscalls, ELF64, and userland shell remain follow-up milestones.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.40-dev

### Added

- Added a bounded x86_64 PIT timer smoke path on normal x86_64 boots.
- Programmed PIT channel 0 at the architecture default timer frequency and unmasked legacy IRQ0 after the IRQ gates and PIC remap are in place.
- Added quiet PIT IRQ handling that increments an x86_64 timer tick counter and sends PIC EOI without flooding serial output.
- Added serial markers for `x86_64 PIT timer online`, `PIT IRQ0 unmasked: 1`, `PIT interrupts after: 1`, `PIT waited ticks: 3`, and `PIT timer ok: 1`.

### Changed

- Kept the opt-in IRQ/exception self-test path separate from the normal PIT smoke path.
- Used a bounded `pause` loop for timer smoke waiting so failed timer delivery reports `PIT timer ok: 0` instead of intentionally parking the CPU.
- Updated Liam_OS version to `0.8.40-dev`.

### Notes

- This is the first normal x86_64 boot milestone that enables hardware interrupts for PIT delivery.
- Keyboard, heap/VMM, PMM-backed page-table allocation, scheduler/process, syscalls, ELF64, and userland shell remain follow-up milestones.
- The default stable boot path remains `ARCH=i386` through `make` and `make run` until x86_64 reaches feature parity.

## Core 0.8.39-dev

### Added

- Added an opt-in x86_64 IRQ self-test boot flag, `liam.x86_64.irq_test=32`.
- Added a software-interrupt validation path that triggers vector 32 through the installed IRQ gate, returns through the IRQ stub, and then continues boot-test execution.
- Added serial markers for `IRQ self-test requested: 1`, `x86_64 IRQ received`, `IRQ vector: 0x0000000000000020`, `IRQ self-test delivered: 1`, and `IRQ self-test returned: 1`.

### Changed

- Extended the x86_64 exception-test GRUB entry so it first validates the IRQ gate/return path, then runs the existing invalid-opcode exception self-test.
- Updated Liam_OS version to `0.8.39-dev`.

### Notes

- This is a software interrupt test only. Hardware IRQ delivery remains masked and interrupts remain disabled during normal boots.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.38-dev

### Added

- Added x86_64 IRQ stubs for legacy IRQ vectors 32 through 47.
- Installed guarded IDT interrupt gates for the full legacy IRQ vector window while keeping hardware IRQ delivery masked.
- Added serial IRQ diagnostics for `IRQ gates installed: 16` and `IRQ gates ok: 1`.
- Added a default x86_64 IRQ handler that records unexpected IRQ delivery and sends legacy PIC EOI when needed.

### Changed

- Tightened x86_64 IRQ policy validation so `IRQ policy ok: 1` now requires installed IRQ gates, disabled interrupts, remapped PIC state, and fully masked PIC lines.
- Updated Liam_OS version to `0.8.38-dev`.

### Notes

- This milestone prepares default IRQ handling without unmasking PIC lines or enabling interrupts yet.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.37-dev

### Added

- Added early x86_64 legacy PIC remap and mask initialization during IDT setup.
- Added serial IRQ-policy markers for `IRQ legacy PIC remapped: 1`, `IRQ master mask: 0x000000FF`, `IRQ slave mask: 0x000000FF`, and `IRQ all masked: 1`.

### Changed

- Tightened `IRQ policy ok: 1` so it now requires interrupts to remain disabled, legacy PIC remapping to complete, and both PIC masks to remain fully closed.
- Updated Liam_OS version to `0.8.37-dev`.

### Notes

- This milestone prepares the legacy IRQ delivery path without enabling hardware interrupts yet.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.36-dev

### Added

- Added x86_64 IRQ policy diagnostics during early IDT setup.
- Added serial markers for `IRQ interrupts enabled: 0`, `IRQ interrupts guarded: 1`, planned legacy IRQ vector base/count, planned PIT and keyboard IRQ vectors, `IRQ APIC deferred: 1`, and `IRQ policy ok: 1`.

### Changed

- Kept hardware interrupts disabled while documenting the planned legacy IRQ vector layout for later PIC/APIC/PIT work.
- Updated Liam_OS version to `0.8.36-dev`.

### Notes

- This milestone makes the interrupt strategy explicit before enabling IRQ delivery.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.35-dev

### Added

- Added bounded Multiboot2 command-line parsing for the x86_64 boot path.
- Added an opt-in x86_64 invalid-opcode exception self-test selected with `liam.x86_64.exception_test=ud2`.
- Added `core/boot/grub-x86_64-exception-test.cfg` for a separate exception-test ISO boot entry.
- Added `make x86_64-exception-test-iso` and `make x86_64-exception-test-run`.

### Changed

- Exposed the exception self-test request through a narrow architecture-local boot flag consumed by the IDT setup path.
- Updated Liam_OS version to `0.8.35-dev`.

### Notes

- Normal i386 and x86_64 boot paths do not deliberately fault.
- The exception-test path should report `Exception self-test requested: 1`, `Name: invalid opcode`, `Vector: 0x0000000000000006`, and `x86_64 panic halt` over serial.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.34-dev

### Added

- Added explicit x86_64 exception panic-halt readiness diagnostics.
- Added serial markers for `IDT panic halt ready: 1` and `IDT panic cli before hlt: 1`.
- Added a shared exception halt helper that prints the panic halt mode before entering the `cli; hlt` loop.

### Changed

- Routed x86_64 CPU exception termination through the shared halt helper instead of open-coding the halt loop in the handler.
- Updated Liam_OS version to `0.8.34-dev`.

### Notes

- This milestone prepares the panic halt path for later intentional exception tests without deliberately faulting the current boot path.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.33-dev

### Fixed

- Fixed the i386 kernel build ordering so `kernel/userland/userland_images.asm` now depends on the generated `build/userland/*.bin` files before NASM evaluates its `incbin` directives.

### Changed

- Added an explicit `USERLAND_IMAGES_OBJ` Makefile target for the embedded userland image object.
- Updated Liam_OS version to `0.8.33-dev`.

### Notes

- This restores `make clean && make` from a clean tree after the recent x86_64 migration batch.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.32-dev

### Added

- Added x86_64 page-fault diagnostic readiness reporting for CR2 fault-address capture and page-fault error-code decoding.
- Added serial markers for `IDT PF CR2 reporting: 1`, `IDT PF error decode: 1`, and `IDT diagnostics ok: 1`.
- Added page-fault exception-path serial decoding for present/write/user/reserved-bit/instruction-fetch error-code flags.

### Changed

- Updated x86_64 documentation and validation notes to include the guarded higher-half runtime entry plus page-fault diagnostic readiness markers.
- Updated Liam_OS version to `0.8.32-dev`.

### Notes

- This milestone does not intentionally trigger a page fault yet; it prepares the reporting path before adding deliberate exception tests.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.
