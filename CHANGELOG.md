# Liam_OS Changelog

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
