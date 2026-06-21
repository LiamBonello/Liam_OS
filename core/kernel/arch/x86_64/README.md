# Liam_OS x86_64 Architecture Scaffold

This directory is reserved for the real x86_64 kernel path. It is intentionally not wired into the default bootable build yet.

The i386 path in `core/kernel/arch/i386/` remains the known-good baseline while this directory grows in small, testable stages.

## Initial milestones

1. Add x86_64 build metadata without changing the default i386 build.
2. Add a separate linker script and boot objects for the x86_64 path.
3. Enter long mode from a 32-bit bootstrap.
4. Bring up a minimal 64-bit console path.
5. Add x86_64 GDT/IDT/interrupt handling.
6. Port paging and memory layout to 64-bit addresses.
7. Revisit syscalls and userspace after the 64-bit kernel path is stable.

## Guardrails

- Do not copy i386 files and rename them as if that made them 64-bit safe.
- Keep i386 bootable until x86_64 has its own verified boot path.
- Keep architecture-specific code under architecture-specific directories.
- Promote shared abstractions only after both i386 and x86_64 need the same contract.
