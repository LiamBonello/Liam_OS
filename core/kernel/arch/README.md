# Liam_OS Architecture Status

The active development target is x86_64. New kernel, userspace, desktop-service, and smoke-test work should land on the x86_64 path first.

i386 is retained as a legacy reference implementation. It is useful for comparing earlier protected-mode, paging, interrupt, syscall, VFS, and flat-userland behavior, but it is not the product target for modern PCs.

## Build Targets

From the repository root:

```sh
make
make run
make check-x86_64
```

These commands target x86_64.

Legacy i386 commands remain available for reference work:

```sh
make legacy-i386
make legacy-i386-run
make legacy-i386-debug
```

## Development Rule

Prefer x86_64 for new functionality. Only change i386 when preserving reference behavior, fixing breakage that blocks the repository, or intentionally removing the legacy path later.
