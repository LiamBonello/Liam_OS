# Liam_OS Changelog

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
- Added smoke-test assertions for the active mapping probes.

### Changed

- Updated the x86_64 boot stage to `Stage: paging probes + descriptor`.
- Updated Liam_OS version to `0.8.26-dev`.

### Notes

- This proves the active C-built page tables can access the kernel image through all planned early aliases.
- This still does not jump execution to the higher-half kernel address yet.
- The default stable boot path remains `ARCH=i386` through `make` and `make run`.

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

## Earlier History

Earlier milestones are preserved in the repository history. Core 0.8.21-dev and earlier covered x86_64 smoke automation, critical IST routing, maintained GDT/TSS loading, bootstrap paging diagnostics, the isolated x86_64 PMM allocator, Multiboot2 parsing, initial long-mode C entry, and the original i386 Core kernel milestones.
