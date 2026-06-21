# Liam_OS Changelog

## Core 0.8.16-dev

### Added

- Added `core/kernel/arch/x86_64/gdt.h` and `gdt.c` to capture bootstrap GDT state from C.
- Added named x86_64 bootstrap GDT symbols for the null, code, data, and end descriptors.
- Added VGA and COM1 serial diagnostics for GDTR, active segment selectors, and descriptor values.

### Changed

- Updated the x86_64 C entry to report `Stage: descriptor + paging + PMM`.
- Wired the x86_64 GDT diagnostics into `make x86_64-kernel`, `make x86_64-iso`, and `make x86_64-run`.
- Updated `make x86_64-info` to describe the descriptor diagnostics milestone.
- Updated Liam_OS version to `0.8.16-dev`.

### Notes

- This still uses the bootstrap GDT; it makes descriptor state explicit before introducing a maintained x86_64 descriptor/TSS module.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.15-dev

### Added

- Added `core/kernel/arch/x86_64/paging.h` and `paging.c` to capture bootstrap paging state from C.
- Added named x86_64 bootstrap paging table symbols for the PML4, PDPT, and PD tables.
- Added VGA and COM1 serial diagnostics for the bootstrap CR3 and huge-page identity map.

### Changed

- Updated the x86_64 C entry to report `Stage: paging baseline + PMM`.
- Wired the x86_64 paging diagnostics into `make x86_64-kernel`, `make x86_64-iso`, and `make x86_64-run`.
- Updated `make x86_64-info` to describe the bootstrap paging baseline milestone.
- Updated Liam_OS version to `0.8.15-dev`.

### Notes

- This still uses the temporary bootstrap identity map; it makes that state explicit before replacing it with a permanent x86_64 paging model.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.14-dev

### Added

- Added `core/kernel/arch/x86_64/pmm.h` and `pmm.c` as an isolated bounded page-stack allocator for early x86_64 physical pages.
- Added an x86_64 PMM allocate/free smoke test with VGA and COM1 serial diagnostics.
- Added x86_64 PMM diagnostics for tracked pages, free pages, dropped pages, duplicate-free rejects, lowest page, highest page, and the smoke-test page.

### Changed

- Wired the x86_64 PMM allocator into `make x86_64-kernel`, `make x86_64-iso`, and `make x86_64-run`.
- Updated the x86_64 C entry to report `Stage: PMM allocator smoke test`.
- Updated x86_64 exception VGA diagnostics to leave room for allocator smoke-test output while preserving full exception details over serial.
- Updated `make x86_64-info` to describe the allocator smoke-test milestone.
- Updated Liam_OS version to `0.8.14-dev`.

### Fixed

- Hardened the early x86_64 PMM free path against duplicate frees, invalid pages, unaligned pages, and obvious out-of-range pages.
- Avoided an overflow-prone page-range loop condition while building the allocator free stack.

### Notes

- This allocator is intentionally isolated from the i386 runtime and is not yet connected to x86_64 paging, heap, processes, syscalls, or userspace.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.13-dev

### Added

- Added retained x86_64 Multiboot2 memory-map regions to the boot summary.
- Added `core/kernel/arch/x86_64/pmm_plan.h` and `pmm_plan.c` for planning-only physical memory management diagnostics.
- Added PMM plan reporting for usable region count, first planned free page, managed page count, managed bytes, and reserved-below boundary.

### Changed

- Extended the x86_64 boot context to include the PMM plan.
- Updated the x86_64 C entry to report `Stage: PMM plan + boot context`.
- Updated `make x86_64-info` to describe the PMM planning milestone.
- Updated Liam_OS version to `0.8.13-dev`.

### Notes

- This does not enable a real x86_64 physical page allocator yet; it validates the memory-map interpretation and allocator boundary first.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.12-dev

### Added

- Added `core/kernel/arch/x86_64/boot_context.h` and `boot_context.c` to preserve parsed boot state in an architecture-owned boot context.
- Added `core/kernel/arch/x86_64/memory_layout.h` and `memory_layout.c` for initial x86_64 memory-layout constants.
- Exposed x86_64 linker-provided kernel image start and end symbols.
- Added x86_64 boot diagnostics for kernel image bounds, kernel image size, and bootstrap identity-map span.

### Changed

- Updated the x86_64 C entry to consume a boot context instead of a loose boot-info summary.
- Cleaned x86_64 bootstrap assembly warnings by using BSS-safe alignment and explicit 64-bit boot-state reads.
- Updated `make x86_64-info` to describe the boot-context milestone.
- Updated Liam_OS version to `0.8.12-dev`.

### Notes

- This milestone prepares the x86_64 path for PMM and paging work by making early memory facts explicit.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

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

## Core 0.8.6-dev

### Added

- Added `core/linker.x86_64.ld` for the experimental ELF64 kernel artifact layout.
- Added `make x86_64-kernel` to build `core/build/x86_64/kernel.elf` without changing the i386 ISO path.
- Documented the experimental x86_64 artifact target in the root README and x86_64 architecture notes.

### Changed

- Updated `make x86_64-info` to report the new artifact target and next migration milestone.
- Updated Liam_OS version to `0.8.6-dev`.

### Notes

- The x86_64 artifact is not bootable yet. It is a controlled build milestone before adding a real boot handoff and long-mode transition.
- The default bootable path remains `ARCH=i386` through GRUB Multiboot.

## Core 0.8.5-dev

### Added

- Added `core/kernel/arch/x86_64/` as the initial architecture scaffold for the future 64-bit kernel path.
- Added `docs/core/x86_64-migration-plan.md` with a staged plan from scaffold to long-mode boot and later userspace work.
- Added Makefile architecture metadata for `i386` and `x86_64`.
- Added `make x86_64-info` to describe the current x86_64 migration status.

### Changed

- Kept `ARCH=i386` as the default and only bootable build path.
- Made accidental `ARCH=x86_64` builds fail clearly until a real x86_64 bootstrap exists.
- Updated Liam_OS version to `0.8.5-dev`.

### Notes

- This milestone does not claim x86_64 boot support yet.
- The next x86_64 milestone should add a separate linker script and long-mode bootstrap experiment while preserving the i386 ISO path.

## Core 0.8.4-dev

### Added

- Added `/bin/sysbadptr` as a userspace bad-pointer syscall regression test.
- Added `docs/core/i386-architecture-assumptions.md` to document current i386 boot, memory, syscall, and userspace assumptions before the x86_64 migration.

### Changed

- Hardened syscall pointer handling for `write`, `open`, `read`, `stat`, `get_arg`, and `exec` by copying user data through kernel-owned buffers after ring-3 range checks.
- Updated `/bin/help` to list `/bin/sysbadptr`.
- Updated Liam_OS version to `0.8.4-dev`.

### Fixed

- Rejected duplicate PMM frees so one physical page cannot be reinserted into the free stack twice.
- Rolled back partially mapped heap growth attempts when a later page allocation or mapping fails.

### Notes

- `/bin/sysbadptr` should return clean errors for invalid syscall pointers without causing a page fault, panic, or hang.
- This is a kernel-hardening milestone before deeper process isolation and x86_64 boot work.

## Core 0.8.3-dev

### Added

- Added `process_run_all_ready`.
- Added kernel shell command `run-all-ready`.
- Updated `/bin/sh` to create multiple READY child processes through `LIAM_SYSCALL_EXEC`.

### Changed

- Updated `/bin/help` to list `run-all-ready`.
- Updated Liam_OS version to `0.8.3-dev`.

### Notes

- `run-all-ready` safely drains READY userspace processes after returning to the kernel shell.
- This is a scheduler stepping stone before automatic userspace process scheduling.

## Core 0.8.2-dev

### Added

- Added mutable process lookup by PID.
- Added lookup for the next READY process.
- Added `process_run_next_ready`.
- Added `process_run_by_pid`.
- Added kernel shell command `run-ready`.
- Added kernel shell command `run-pid <pid>`.

### Changed

- Updated `/bin/help` to list `run-ready` and `run-pid`.
- Updated Liam_OS version to `0.8.2-dev`.

### Notes

- This allows READY child processes created through `LIAM_SYSCALL_EXEC` to be executed safely after the parent returns to the kernel shell.
- This is a controlled stepping stone before automatic userspace process scheduling.

## Core 0.8.1-dev

### Added

- Added `LIAM_SYSCALL_EXEC`.
- Added kernel syscall dispatch support for userspace-driven process spawning.
- Updated `/bin/sh` to create a `/bin/echo` child process through `LIAM_SYSCALL_EXEC`.
- `LIAM_SYSCALL_EXEC` creates a READY child process but does not run it synchronously from inside the syscall.

### Changed

- Updated `/bin/help` to describe `/bin/sh` as a userspace shell syscall demo.
- Updated Liam_OS version to `0.8.1-dev`.

### Notes

- This is the first milestone where a userspace program requests execution of another userspace program.
- `/bin/sh` is still non-interactive; keyboard/input syscalls are required before it can become a real interactive shell.

## Core 0.8.0-dev

### Added

- Added `core/userland/flat/sh/sh.asm`.
- Added `/bin/sh` as a userspace shell stub.
- Added `sh.bin` to the generated userland build pipeline.
- Added `/bin/sh` to initramfs and user image registration.

### Changed

- Updated `/bin/help` to list `/bin/sh`.
- Updated Liam_OS version to `0.8.0-dev`.

### Notes

- `/bin/sh` is currently a non-interactive userspace shell stub.
- Interactive shell behavior requires future input and exec syscalls.

## Core 0.7.1-dev

### Added

- Added `core/userland/flat/args/args.asm`.
- Added `/bin/args` as an argv inspection program.
- Added `core/userland/flat/echo/echo.asm`.
- Added `/bin/echo` as an argument-aware userspace program.
- Added `args.bin` and `echo.bin` to the generated userland build pipeline.
- Added `/bin/args` and `/bin/echo` to initramfs and user image registration.

### Changed

- Updated `/bin/help` to list `/bin/args` and `/bin/echo`.
- Updated Liam_OS version to `0.7.1-dev`.

### Notes

- `/bin/args` validates `LIAM_SYSCALL_GET_ARGC` and `LIAM_SYSCALL_GET_ARG`.
- `/bin/echo` validates multi-argument userspace execution.

## Core 0.7.0-dev

### Added

- Added kernel-side process argument storage.
- Added argument-aware process spawning through `exec_spawn_with_args`.
- Added `LIAM_SYSCALL_GET_ARGC`.
- Added `LIAM_SYSCALL_GET_ARG`.
- Added argument parsing for the shell `exec` command.

### Changed

- Updated `/bin/cat` to read `argv[1]` when provided.
- `/bin/cat` now defaults to `/etc/os-release` when no path argument is provided.
- Updated `/bin/help` to document argument-aware `/bin/cat`.
- Updated Liam_OS version to `0.7.0-dev`.

### Notes

- This is the first argument-aware userspace milestone.
- The current argv model is kernel-managed and exposed through syscalls rather than an initial user stack ABI.

## Core 0.6.4-dev

### Added

- Added `core/userland/flat/uptime/uptime.asm`.
- Added `/bin/uptime` as a flat userspace program.
- Added `uptime.bin` to the userland build pipeline.
- Added `/bin/uptime` to initramfs and user image registration.

### Changed

- Updated `/bin/help` to list `/bin/uptime`.
- Updated Liam_OS version to `0.6.4-dev`.

### Notes

- `/bin/uptime` currently prints raw kernel ticks from `LIAM_SYSCALL_GET_TICKS`.
- A later timer milestone can expose uptime in seconds once timer frequency is formalized.

## Core 0.6.3-dev

### Added

- Added `core/userland/flat/help/help.asm`.
- Added `/bin/help` as a flat userspace program.
- Added `help.bin` to the generated userland build pipeline.
- Added `/bin/help` to initramfs and user image registration.

### Changed

- Updated Liam_OS version to `0.6.3-dev`.

### Notes

- `/bin/help` provides a userspace command reference for the current shell and userspace programs.
- This further validates the generated flat userland build pipeline introduced in Core 0.6.1-dev.

## Core 0.6.2-dev

### Added

- Added `core/userland/flat/clear/clear.asm`.
- Added `/bin/clear` as a flat userspace program.
- Added `clear.bin` to the generated userland build pipeline.

### Changed

- Updated Liam_OS version to `0.6.2-dev`.

### Notes

- This validates that new flat userland programs can now be added through `USERLAND_FLAT_NAMES` plus normal initramfs/image registration.
- `/bin/clear` currently emits ANSI clear-screen escape bytes.

## Core 0.6.1-dev

### Changed

- Replaced per-program flat userland Makefile rules with one generic pattern rule.
- Added `USERLAND_FLAT_NAMES` as the source of truth for flat userland program names.
- Derived `USERLAND_FLAT_SRC` and `USERLAND_FLAT_BIN` from `USERLAND_FLAT_NAMES`.
- Updated Liam_OS version to `0.6.1-dev`.

### Notes

- New flat userland programs can now be added by creating `core/userland/flat/<name>/<name>.asm` and adding `<name>` to `USERLAND_FLAT_NAMES`.
- The kernel embedding path remains unchanged through `core/kernel/userland/userland_images.asm`.

## Core 0.6.0-dev

### Added

- Added `core/userland/flat/init/init.asm`.
- Added `init.bin` to the userland build pipeline.
- Added `/sbin/init` embedding through `core/kernel/userland/userland_images.asm`.

### Changed

- Moved `/sbin/init` out of `ring3_entry.asm`.
- `/sbin/init` is now built as `build/userland/init.bin`.
- `ring3_entry.asm` now contains only ring 3 transition, syscall entry, and kernel return support.
- Updated Liam_OS version to `0.6.0-dev`.

### Notes

- All current flat userspace programs are now built from `core/userland/flat/*`.
- `ring3_entry.asm` no longer embeds userspace payloads.

## Core 0.5.4-dev

### Added

- Added `core/userland/flat/os-release/os-release.asm`.
- Added `core/userland/flat/cat/cat.asm`.
- Added `os-release.bin` and `cat.bin` to the userland build pipeline.
- Added `/bin/os-release` and `/bin/cat` embedding through `core/kernel/userland/userland_images.asm`.

### Changed

- Moved `/bin/os-release` out of `ring3_entry.asm`.
- Moved `/bin/cat` out of `ring3_entry.asm`.
- `/bin/os-release` is now built as `build/userland/os-release.bin`.
- `/bin/cat` is now built as `build/userland/cat.bin`.
- Updated Liam_OS version to `0.5.4-dev`.

### Notes

- `/bin/hello`, `/bin/about`, `/bin/pid`, `/bin/ticks`, `/bin/syscheck`, `/bin/os-release`, and `/bin/cat` are now built from separate userland source files.
- `ring3_entry.asm` now only retains `/sbin/init` as an embedded userspace image.

## Core 0.5.3-dev

### Added

- Added `core/userland/flat/syscheck/syscheck.asm`.
- Added `syscheck.bin` to the userland build pipeline.
- Added `/bin/syscheck` embedding through `core/kernel/userland/userland_images.asm`.

### Changed

- Moved `/bin/syscheck` out of `ring3_entry.asm`.
- `/bin/syscheck` is now built as `build/userland/syscheck.bin`.
- Updated Liam_OS version to `0.5.3-dev`.

### Notes

- `/bin/hello`, `/bin/about`, `/bin/pid`, `/bin/ticks`, and `/bin/syscheck` are now built from separate userland source files.
- `ring3_entry.asm` now only retains `/sbin/init`, `/bin/os-release`, and `/bin/cat` as embedded userspace images.

## Core 0.5.2-dev

### Added

- Added `core/userland/flat/pid/pid.asm`.
- Added `core/userland/flat/ticks/ticks.asm`.
- Added `pid.bin` and `ticks.bin` to the userland build pipeline.
- Added `/bin/pid` and `/bin/ticks` embedding through `core/kernel/userland/userland_images.asm`.

### Changed

- Moved `/bin/pid` out of `ring3_entry.asm`.
- Moved `/bin/ticks` out of `ring3_entry.asm`.
- `/bin/pid` is now built as `build/userland/pid.bin`.
- `/bin/ticks` is now built as `build/userland/ticks.bin`.
- Updated Liam_OS version to `0.5.2-dev`.

### Notes

- `/bin/hello`, `/bin/about`, `/bin/pid`, and `/bin/ticks` are now built from separate userland source files.
- `ring3_entry.asm` now only retains `/sbin/init`, `/bin/syscheck`, `/bin/os-release`, and `/bin/cat` as embedded userspace images.

## Core 0.5.1-dev

### Added

- Added `core/userland/flat/about/about.asm`.
- Added `about.bin` to the userland build pipeline.
- Added `/bin/about` embedding through `core/kernel/userland/userland_images.asm`.

### Changed

- Moved `/bin/about` out of `ring3_entry.asm`.
- `/bin/about` is now built as `build/userland/about.bin`.
- Updated Liam_OS version to `0.5.1-dev`.

### Notes

- `/bin/hello` and `/bin/about` are now both built from separate userland source files.
- `ring3_entry.asm` is gradually being reduced back toward ring 3 transition code and legacy embedded userspace only.

## Core 0.5.0-dev

### Added

- Added initial `core/userland/` source tree.
- Added `core/userland/flat/hello/hello.asm` as a separately built userspace program.
- Added `core/kernel/userland/userland_images.asm` to embed built userland binaries into the kernel image.
- Added userland build rules to `core/Makefile`.
- Added `make -C core userland` target.

### Changed

- Moved `/bin/hello` out of `ring3_entry.asm`.
- `/bin/hello` is now built as `build/userland/hello.bin`.
- Updated Liam_OS version to `0.5.0-dev`.
- Updated `/bin/about` version output to `0.5.0-dev`.

### Notes

- This is the first step toward a proper userspace build pipeline.
- `/bin/hello` is still a flat binary, but it is now built from a separate userland source file.
- Future milestones should migrate more commands out of `ring3_entry.asm`.

## Core 0.4.4-dev

### Added

- Added `/bin/cat` as a userspace command.
- Registered `/bin/cat` in the userspace image registry.
- Added `/bin/cat` to the static initramfs.
- Updated shell help text with `/bin/cat`.

### Changed

- Updated `/etc/os-release` version to `0.4.4-dev`.
- Updated `/bin/about` version output to `0.4.4-dev`.

### Notes

- `/bin/cat` currently reads `/etc/os-release` by default because userspace command arguments are not implemented yet.
- This milestone prepares for future `argc`/`argv` support.

## Core 0.4.3-dev

### Added

- Added `/bin/os-release` as a userspace command.
- Registered `/bin/os-release` in the userspace image registry.
- Added `/bin/os-release` to the static initramfs.
- Added a normal userspace file-read test for `/etc/os-release`.
- Updated shell help text with the new userspace command.

### Notes

- `/bin/os-release` validates `LIAM_SYSCALL_OPEN`, `LIAM_SYSCALL_READ`, `LIAM_SYSCALL_CLOSE`, and `LIAM_SYSCALL_WRITE` from a standalone userspace command.
- This proves file reading works outside the initial `/sbin/init` userspace process.

## Core 0.4.2-dev

### Added

- Added `/bin/pid` as a userspace command.
- Added `/bin/ticks` as a userspace command.
- Added `/bin/about` as a userspace command.
- Registered `/bin/pid`, `/bin/ticks`, and `/bin/about` in the userspace image registry.
- Added `/bin/pid`, `/bin/ticks`, and `/bin/about` to the static initramfs.

### Notes

- `/bin/pid` prints the PID returned by `LIAM_SYSCALL_GET_PID`.
- `/bin/ticks` prints the timer value returned by `LIAM_SYSCALL_GET_TICKS`.
- `/bin/about` prints static Liam_OS Core build information.

## Core 0.4.1-dev

### Added

- Updated `/bin/syscheck` to print numeric syscall return values.
- Added userspace decimal integer printing inside the syscheck flat binary.
- `/bin/syscheck` now prints the PID returned by `LIAM_SYSCALL_GET_PID`.
- `/bin/syscheck` now prints the tick count returned by `LIAM_SYSCALL_GET_TICKS`.

### Notes

- This confirms userspace can consume syscall return values, convert them to decimal text, and write them back through `LIAM_SYSCALL_WRITE`.
- Numeric output is currently implemented inside the assembly-based flat userspace image.

## Core 0.4.0-dev

### Added

- Added `/bin/syscheck` as a userspace syscall validation program.
- Added validation of `LIAM_SYSCALL_GET_PID` from userspace.
- Added validation of `LIAM_SYSCALL_GET_TICKS` from userspace.
- Registered `/bin/syscheck` in the userspace image registry.
- Added `/bin/syscheck` to the static initramfs.

### Notes

- This is the first Core 0.4 userspace syscall expansion milestone.
- The program verifies syscall availability but does not yet print numeric syscall return values.

## Core 0.3.3-dev

### Added

- Added retained last-exited process result tracking.
- Added `wait-last` shell command.

### Notes

- `wait-last` keeps the last process result available even after exited process slots are cleared.
- This is not full blocking `waitpid` behavior yet.

## Core 0.3.1-dev

### Added

- Added process slot reset helper.
- Added exited/failed process slot cleanup.
- Added `proc-clear-exited` shell command.

### Notes

- Exited and failed process slots can now be reclaimed without rebooting.
- PID values are not recycled yet.
- Running processes are not cleared.

## Core 0.3.0-dev

### Added

- Added process subsystem statistics.
- Added process create/run/exit counters.
- Added `proc-status` shell command.
- Added `proc-info <pid>` shell command.
- Improved `ps` status output to print status names instead of raw numeric values.

### Notes

- Process slots are still retained after exit.
- There is no process reaping or PID recycling yet.

## Core 0.2.4-dev

### Added

- Added ELF32 executable loading through `elf32_load_from_node`.
- Added PT_LOAD program-header selection.
- Added ELF32 segment bounds validation.
- Added `/bin/hello-elf` as an executable ELF32 initramfs program.
- Registered `/bin/hello-elf` in the userspace image registry.
- Updated `elf-status` with ELF32 load statistics.

### Notes

- ELF32 execution now works for a single `PT_LOAD` segment loaded at `0x10000000`.
- The current ELF loader intentionally supports only the early ring 3 one-page userspace model.
- Dynamic linking, relocations, multiple load segments, and per-process address spaces are not supported yet.

## Core 0.2.1-dev

### Added

- Added ELF32 loader foundation.
- Added ELF32 header and program-header definitions.
- Added ELF32 validation support for i386 executable files.
- Added `/tests` directory to the static initramfs.
- Added `/tests/elf32-sample` as a validation fixture.
- Added `elf-status` shell command.
- Added `elf-check <path>` shell command.

### Notes

- ELF files can now be inspected and validated.
- ELF execution is not enabled yet.
- Existing flat userspace image execution remains unchanged.

## Core 0.2.0-dev

### Added

- Added `/bin` directory to the static initramfs.
- Added `/bin/hello` as a second flat userspace image.
- Registered `/bin/hello` in the userspace image registry.
- Updated shell help text with `/bin/hello` execution example.
- Added Core 0.1 boot milestone documentation.

## Core 0.1.0

### Added

- GRUB Multiboot boot path.
- i386 protected-mode kernel entry.
- GDT, TSS, IDT, PIC, and PIT setup.
- VGA text console.
- Keyboard driver.
- Physical memory manager.
- Virtual memory manager and paging.
- Kernel heap.
- Basic scheduler primitives.
- Process table.
- Ring 3 userspace transition.
- `int 0x80` syscall dispatcher.
- Static initramfs.
- `/sbin/init`.
- `/etc/os-release`.
- Liam OS shell.
