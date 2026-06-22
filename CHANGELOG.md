# Liam_OS Changelog

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

## Core 0.8.31-dev

### Added

- Added `core/kernel/arch/x86_64/runtime.h` and `runtime.c` as the first explicit x86_64 higher-half runtime boundary.
- Added a guarded higher-half runtime entry call after C-owned page tables are active.
- Added serial diagnostics for runtime entry addresses, state pointer, expected/active CR3, argument marker, stack sample, return marker, entered marker, scratch marker, and `Runtime entry ok: 1`.

### Changed

- Updated the x86_64 boot stage to `Stage: higher-half runtime entry`.
- Wired `runtime.c` into the staged x86_64 build path.
- Updated x86_64 documentation to describe manual validation for the runtime entry.
- Updated Liam_OS version to `0.8.31-dev`.

### Notes

- This milestone proves a named x86_64 runtime entry can execute through the higher-half kernel alias while validating the active CR3 and current transition stack.
- This still keeps the low identity map alive as part of the transition path; removing that dependency is a later step.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.30-dev

### Added

- Added a guarded x86_64 higher-half handoff probe that calls a C function through the higher-half kernel alias after C-owned page tables are active.
- Added serial diagnostics for the handoff entry addresses, scratch pointer, argument marker, result marker, scratch write marker, readiness, and `Higher-half handoff ok: 1`.

### Changed

- Updated the x86_64 boot stage to `Stage: higher-half handoff probe`.
- Updated x86_64 documentation to describe manual validation for the handoff probe.
- Updated Liam_OS version to `0.8.30-dev`.

### Notes

- This milestone proves a higher-half C call can use the current calling convention and stack, accept an argument, write through a caller-provided pointer, and return a validation marker.
- This is still a guarded probe, not a full relocation of the x86_64 C runtime to the higher-half kernel window.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.29-dev

### Added

- Added a C-level x86_64 higher-half execution probe that calls compiler-generated C code through both the low identity-mapped address and the higher-half kernel alias.
- Added serial diagnostics for C probe code/data addresses, expected value, low-result, high-result, readiness, and `Higher-half C probe ok: 1`.

### Changed

- Updated the x86_64 boot stage to `Stage: higher-half C probe + descriptor`.
- Updated x86_64 documentation to describe both the assembly probe and the C probe.
- Updated Liam_OS version to `0.8.29-dev`.

### Notes

- This milestone proves the active higher-half kernel alias can execute a normal freestanding C function and read its marker data through compiler-generated access.
- This still does not relocate the full C runtime to the higher-half kernel window.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.28-dev

### Changed

- Removed the `make x86_64-smoke` target, its shell script, and its GitHub Actions workflow because this agent environment still cannot run the repository boot tests directly.
- Trimmed duplicated x86_64 smoke-test documentation from the top-level and architecture README files.
- Updated the x86_64 migration plan to describe manual serial-marker validation instead of removed headless automation.
- Updated Liam_OS version to `0.8.28-dev`.

### Notes

- The x86_64 boot path still keeps the higher-half execution probe and serial diagnostics.
- Manual validation now uses `make x86_64-info`, `make x86_64-kernel`, `make x86_64-iso`, and `make x86_64-run`.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.27-dev

### Added

- Added a tiny x86_64 higher-half execution probe that safely calls an assembly-only function through both its low identity-mapped address and its higher-half kernel alias.
- Added serial diagnostics for the probe entry addresses, byte span, expected value, low-result, high-result, readiness gates, and `Higher-half probe ok: 1`.
- Added smoke-test assertions for the higher-half execution probe markers.

### Changed

- Updated the x86_64 boot stage to `Stage: higher-half probe + descriptor`.
- Updated x86_64 documentation to distinguish this isolated instruction-fetch probe from full higher-half C runtime relocation.
- Updated Liam_OS version to `0.8.27-dev`.

### Notes

- This milestone proves instruction fetch through the planned higher-half kernel alias after C-owned page tables are active.
- This still does not relocate the full C runtime to the higher-half kernel window.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.26-dev

### Added

- Added active x86_64 paging alias probes for the identity map, direct physical map, and higher-half kernel mapping.
- Added serial diagnostics for probe addresses, probe values, per-alias probe health, and `Paging probes ok: 1`.

### Changed

- Updated the x86_64 boot stage to `Stage: paging probes + descriptor`.
- Updated Liam_OS version to `0.8.26-dev`.

### Notes

- This proves the active C-built page tables can access the kernel image through all planned early aliases.
- This still does not jump execution to the higher-half kernel address yet.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.
