# Liam_OS Core: Next Kernel Steps

This file defines the next engineering sequence for the custom kernel.

## Immediate priorities

1. Keep the current i386 build stable.
2. Add a small build verification script.
3. Document current syscalls and shell commands.
4. Introduce ELF32 loading behind the existing loader boundary.
5. Add file syscalls after VFS semantics are stable.
6. Add per-process address spaces before expanding multitasking.

## Recommended rule

Do not start graphics, animations, or desktop work inside the custom kernel until these foundations exist:

- ELF executable loading.
- Basic userspace shell.
- Process address-space isolation.
- Readable persistent or initramfs filesystem model.
- Input event delivery.

## Architecture rule

Keep i386 and future x86_64 separated:

```txt
core/kernel/arch/i386/
core/kernel/arch/x86_64/
```

The x86_64 port should be added later as a parallel implementation, not by mutating current i386 code into a half-ported state.
