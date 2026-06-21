# Liam_OS Changelog

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
- Added `uptime.bin` to the generated userland build pipeline.
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
- Added `/bin/clear` to initramfs and user image registration.

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
- Updated shell help text with the new userspace commands.

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
- This milestone improves observability before process cleanup and address-space isolation work.

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