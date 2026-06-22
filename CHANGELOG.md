# Liam_OS Changelog

## Core 0.8.25-dev

### Added

- Added a guarded x86_64 paging activation path that switches CR3 to the C-built candidate page tables only after `Paging builder ok: 1`.
- Added serial diagnostics for previous CR3, requested CR3, active CR3, builder readiness, CR3 change detection, active-CR3 matching, and `Paging activation ok: 1`.
- Added smoke-test assertions for active candidate page tables after the CR3 switch.

### Changed

- Updated the x86_64 boot stage to `Stage: paging active + descriptor`.
- Updated Liam_OS version to `0.8.25-dev`.

### Notes

- The x86_64 path now runs on the C-built candidate tables while retaining the transition identity map for the current low-half execution path.
- This still does not relocate execution to the higher-half kernel window or connect the PMM to dynamic page-table allocation.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.24-dev

### Added

- Added `core/kernel/arch/x86_64/paging_builder.h` and `paging_builder.c` to build candidate C-owned x86_64 page tables for the planned memory map.
- Added serial diagnostics for candidate PML4, identity-map, direct-map, and higher-half kernel page-table state.
- Added smoke-test assertions for candidate page-table alignment, PML4 population, identity/direct huge-page coverage, higher-half kernel mapping, and `Paging builder ok: 1`.

### Changed

- Updated the x86_64 boot stage to `Stage: paging builder + descriptor`.
- Wired the x86_64 paging builder into `make x86_64-kernel`, `make x86_64-iso`, and `make x86_64-smoke`.
- Updated Liam_OS version to `0.8.24-dev`.

### Notes

- This milestone still does not switch CR3. It validates the candidate page tables before the x86_64 path starts running on them.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.23-dev

### Added

- Added `core/kernel/arch/x86_64/paging_plan.h` and `paging_plan.c` for a smoke-validated x86_64 virtual memory plan.
- Added serial diagnostics for the planned higher-half kernel window, direct physical map window, transition identity-map window, and their PML4 slots.
- Added smoke-test assertions for canonical virtual windows, distinct PML4 slots, planned-region count, and `VM plan ok: 1`.

### Changed

- Updated the x86_64 boot stage to `Stage: VM plan + descriptor + PMM`.
- Wired the x86_64 virtual memory plan into `make x86_64-kernel`, `make x86_64-iso`, and `make x86_64-smoke`.
- Removed the unused `timeout` tool check from the x86_64 smoke target prerequisites.
- Updated Liam_OS version to `0.8.23-dev`.

### Notes

- This milestone does not switch CR3 or replace the bootstrap identity map yet. It defines and validates the permanent map shape before the future page-table builder starts using it.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.22-dev

### Added

- Added x86_64 CPUID baseline diagnostics to the C boot report.
- Added headless smoke-test validation for required CPU capability markers.

### Changed

- Shifted early VGA diagnostic rows so CPU baseline, IDT IST, PMM smoke, and descriptor summary markers all fit.
- Updated x86_64 documentation to include CPU baseline diagnostics.
- Updated Liam_OS version to `0.8.22-dev`.

### Notes

- The x86_64 CPU baseline currently requires CPUID, FPU, MSR, APIC, SSE, SSE2, SYSCALL/SYSRET, NX, and long mode. 1 GiB pages are reported but not required yet.
- The headless x86_64 smoke target now validates CPU baseline, descriptor/IST, PMM, paging, and GDT/TSS serial markers before passing.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.21-dev

### Added

- Added `core/scripts/x86_64_smoke.sh` to boot the experimental x86_64 ISO headlessly in QEMU and validate required COM1 serial markers.
- Added `make x86_64-smoke` as the automated x86_64 boot target.
- Added `.github/workflows/core-x86_64-smoke.yml` to run the x86_64 headless boot smoke test in GitHub Actions and upload the serial log.

### Changed

- Added serial output for the x86_64 `Desc/IST ok` summary so headless boot tests can validate the same descriptor health marker shown on VGA.
- Updated `make x86_64-info`, README, and x86_64 architecture notes to document the headless smoke target and log location.
- Updated Liam_OS version to `0.8.21-dev`.

### Notes

- The smoke target validates the current x86_64 boot path without requiring a local QEMU window or screenshots.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.20-dev

### Added

- Routed the x86_64 NMI exception gate through IST2.
- Routed the x86_64 page-fault exception gate through IST3.
- Added combined IDT diagnostics for NMI, double-fault, and page-fault IST gate routing.

### Changed

- Loaded the maintained x86_64 GDT/TSS before installing the IST-backed IDT.
- Updated the x86_64 C entry to report `Stage: IST gates + descriptor + PMM`.
- Updated `make x86_64-info` to describe the critical-exception IST milestone.
- Updated Liam_OS version to `0.8.20-dev`.

### Notes

- This milestone validates descriptor routing without intentionally triggering NMI, double fault, or page fault exceptions.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Core 0.8.19-dev

### Added

- Routed the x86_64 double-fault exception gate through IST1.
- Added IDT state diagnostics for IDTR base/limit, installed exception-gate count, double-fault vector, double-fault IST, and double-fault gate presence.
- Added a combined descriptor summary that validates double-fault IST routing plus the maintained GDT/TSS state.

### Changed

- Updated the x86_64 C entry to report `Stage: DF IST + descriptor + PMM`.
- Updated `make x86_64-info` to describe the double-fault IST milestone.
- Updated Liam_OS version to `0.8.19-dev`.

### Notes

- This milestone does not deliberately trigger a double fault; it validates the IDT descriptor routing without risking an intentional fault storm.
- NMI and page-fault IST policy remains a future decision after the descriptor path stays stable.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

## Earlier History

Earlier milestones are preserved in the repository history. Core 0.8.18-dev and earlier covered maintained GDT/TSS loading, TSS planning, bootstrap GDT diagnostics, bootstrap paging diagnostics, the isolated x86_64 PMM allocator, Multiboot2 parsing, initial long-mode C entry, and the original i386 Core kernel milestones.
