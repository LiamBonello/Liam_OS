# Liam_OS Changelog

## Core 0.8.11-dev

### Added

- Added `core/kernel/arch/x86_64/idt.h` and `idt.c` for early x86_64 IDT setup.
- Added `core/kernel/arch/x86_64/idt_stubs.asm` with exception stubs for CPU vectors 0 through 31.
- Added named x86_64 exception diagnostics over VGA and COM1 serial.

### Changed

- Updated the x86_64 C entry to install the early exception IDT before continuing boot diagnostics.
- Updated `make x86_64-info` to describe the IDT and exception milestone.
- Updated Liam_OS version to `0.8.11-dev`.

### Notes

- This milestone does not enable IRQs, APIC, PIC, PIT, scheduling, syscalls, or userspace on x86_64 yet.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.10-dev

### Added

- Added `core/kernel/arch/x86_64/boot_info.h` and `boot_info.c` for Multiboot2 boot-information parsing.
- Parsed Multiboot2 bootloader name, total boot-info size, basic memory info, and memory-map tags.
- Added early x86_64 memory-map summary output over VGA and COM1 serial.
- Extended x86_64 early console helpers with decimal, 32-bit hex, and 64-bit hex formatting.

### Changed

- Updated the x86_64 C entry to report bootloader name, boot-info size, memory-map entry count, and usable memory bytes.
- Updated `make x86_64-info` to describe the Multiboot2 parser milestone.
- Updated Liam_OS version to `0.8.10-dev`.

### Notes

- This larger x86_64 milestone moves beyond raw boot-state printing into real Multiboot2 tag parsing and memory discovery.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.9-dev

### Added

- Added `core/kernel/arch/x86_64/types.h` for local x86_64 fixed-width types.
- Added `core/kernel/arch/x86_64/console.h` and `console.c` for early VGA text output and COM1 serial output.
- Captured GRUB's Multiboot2 magic and boot information pointer in the x86_64 bootstrap.
- Passed the captured Multiboot2 boot state into `kernel_main_x86_64`.

### Changed

- Updated the x86_64 C entry to report `Multiboot2 magic: ok` and the boot-info pointer.
- Updated `make x86_64-run` to route serial output to the terminal with `-serial stdio`.
- Updated Liam_OS version to `0.8.9-dev`.

### Notes

- This is still an experimental x86_64 path, but it now has enough boot-state and serial diagnostics to support the next boot-info parsing and runtime setup work.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.8-dev

### Added

- Added `core/kernel/arch/x86_64/kernel.c` as the first minimal freestanding x86_64 C kernel entry.
- Added x86_64 C object compilation to the experimental `make x86_64-kernel` path.

### Changed

- Updated `core/kernel/arch/x86_64/start.asm` to call `kernel_main_x86_64` after the long-mode handoff.
- Updated `make x86_64-info` to report the new C-entry screen message.
- Updated Liam_OS version to `0.8.8-dev`.

### Notes

- The x86_64 path now proves long-mode assembly can enter freestanding C and write to VGA text memory.
- This still does not initialize the shared kernel runtime, interrupts, heap, processes, syscalls, or userspace.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.7-dev

### Added

- Added `core/kernel/arch/x86_64/boot32.asm` as an experimental Multiboot2 32-bit handoff into x86_64 long mode.
- Added `core/boot/grub-x86_64.cfg` for the experimental x86_64 ISO path.
- Added `make x86_64-iso` to build `core/build/x86_64/liam_os_x86_64.iso`.
- Added `make x86_64-run` to boot the experimental x86_64 ISO with `qemu-system-x86_64`.

### Changed

- Updated the x86_64 linker script to keep the Multiboot2 header at the front of the ELF64 image.
- Updated the x86_64 entry message to `Liam_OS x86_64 long mode online`.
- Updated Liam_OS version to `0.8.7-dev`.

### Notes

- This is an experimental long-mode assembly handoff only. It does not yet initialize the shared C kernel runtime, interrupts, heap, processes, syscalls, or userspace.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.
