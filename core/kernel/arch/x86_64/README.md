# Liam_OS x86_64 Architecture Path

This directory contains the active x86_64 kernel path. Liam_OS is being built around x86_64 as the long-term target architecture, so this directory is the main focus of ongoing work.

The i386 path remains available only as a compatibility fallback while x86_64 reaches feature parity.

## Current status

The x86_64 path builds an experimental ELF64 kernel artifact through:

```sh
cd core
make x86_64-kernel
```

The artifact is written to:

```txt
core/build/x86_64/kernel.elf
```

It can also build and run an experimental Multiboot2 ISO that enters long mode, calls a freestanding x86_64 C entry, loads a maintained GDT/TSS, installs exception and IRQ gates, remaps the legacy PIC, parses Multiboot2 boot data, initializes an architecture-owned boot context, brings up an isolated PMM, builds PMM-backed page tables, switches CR3 to the C-owned tables, validates identity/direct-map/higher-half aliases, initializes a direct-map early heap, programs the fast-syscall MSRs, enters ring 3, dispatches live user syscalls, loads an embedded ELF64 init image, and boots a minimal x86_64 user shell.

```sh
cd core
make x86_64-iso
make x86_64-run
```

The ISO is written to:

```txt
core/build/x86_64/liam_os_x86_64.iso
```

`make x86_64-run` is terminal-interactive. It runs QEMU with serial input and output attached to the terminal, so type shell commands in the same terminal that launched QEMU. The older graphical path remains available through:

```sh
make x86_64-run-gui
```

## Expected normal boot markers

The normal x86_64 run should reach the interactive shell without printing the old subsystem smoke-test banner wall:

```txt
Liam_OS x86_64 shell online
$
```

The interactive shell currently supports:

```txt
help
about
version
pid
ps
deskcheck
wait
echo <text>
ls <path>
cat <path>
stat <path>
exec <path>
clear
exit
```

`help`, `about`, and `version` are read through the VFS. `pid` and `/bin/sysinfo` report the live `getpid` syscall result. `ps` reports a kernel-owned process snapshot. `deskcheck` exercises the desktop-readiness syscalls from the shell. `wait` consumes queued completed child process statuses for the shell.

## Manual validation

Use this path for the normal x86_64 milestone:

```sh
cd core
make clean
make x86_64-info
make x86_64-kernel
make x86_64-iso
make x86_64-run
```

At the shell prompt, test:

```txt
help
about
version
pid
ps
deskcheck
echo hello from x86_64
ls /
cat /usr/share/help.txt
stat /bin/sysinfo
exec /bin/sysinfo
exec /bin/eventd
clear
exit
```

The same boot/shell path can be checked automatically from the repository root:

```sh
make check-x86_64
```

or from this `core/` directory:

```sh
make x86_64-smoke
```

The smoke check builds the x86_64 ISO, boots it in QEMU with serial I/O, runs
`version`, `pid`, `ps`, `exec /bin/windowd`, `wait`, `deskcheck`, and `exit`,
and verifies the expected framebuffer, desktop-services, shell,
session-storage, and process markers. The expected graphics foundation markers
are:

```txt
Framebuffer found: 1
Framebuffer width: 1024
Framebuffer height: 768
Framebuffer bpp: 32
Framebuffer type: 1
Framebuffer RGB format ok: 1
Framebuffer virtual address: 0xFFFF900000000000
Framebuffer mapping ready: 1
Framebuffer surface ready: 1
Framebuffer smoke ok: 1
Desktop scheduler ticks ready: 1
Desktop input ready: 1
Desktop input events ready: 1
Desktop input event capacity: 64
Desktop storage readonly VFS ready: 1
Desktop session storage ready: 1
Desktop persistent storage ready: 0
Desktop graphics ready: 1
Desktop window service ready: 1
Desktop services smoke ok: 1
```

Use this separate opt-in path for the x86_64 IRQ and exception self-test:

```sh
cd core
make clean
make x86_64-exception-test-iso
make x86_64-exception-test-run
```

Check the serial output for:

```txt
IRQ self-test requested: 1
x86_64 IRQ received
IRQ self-test delivered: 1
IRQ self-test returned: 1
Exception self-test requested: 1
x86_64 exception self-test: ud2
x86_64 exception
Name: invalid opcode
Vector: 0x0000000000000006
Error code: 0x0000000000000000
x86_64 panic halt
Panic halt mode: cli; hlt
```

## Kernel readiness before desktop work

The x86_64 path is the right base for desktop work, but it is not ready to be
treated as the finished product kernel yet.

Already in place:

1. Multiboot2 long-mode boot into a freestanding x86_64 C kernel.
2. Architecture-owned boot info, memory map parsing, PMM, page-table builder,
   higher-half/direct-map validation, and early heap.
3. Maintained GDT/TSS, IDT, guarded exception/IRQ path, and fast-syscall MSR
   setup.
4. Ring-3 entry with live syscall dispatch.
5. Minimal VFS-backed x86_64 user shell with `open/read/stat/exec`, `pid`,
   `ps`, and `wait` coverage.
6. Per-process user address-space records with process-owned CR3/page-table
   pages for spawned ELF64 user images.
7. Multiboot2 framebuffer discovery for 1024x768x32 RGB mode and a stable
   higher-half framebuffer mapping for future graphics code.
8. Kernel framebuffer drawing surface with RGB packing, pixel writes,
   rectangle fills, and smoke-draw/readback validation.
9. Desktop-services kernel contract for timer-backed scheduler readiness,
   keyboard input and input-event readiness, read-only VFS storage capability, graphics
   readiness, and userspace window-present requests.
10. `/bin/windowd` userspace probe for desktop status and demo window drawing.
11. Timer, sleep, input-status, and writable RAM-backed session-storage
    syscalls with `/bin/timed`, `/bin/inputd`, and `/bin/storaged` probes.
12. Structured, cursor-based input-event syscall plumbing with `/bin/eventd`
    probe coverage.
13. Shell-level `deskcheck` validation for desktop-readiness syscalls.
14. Automated x86_64 QEMU smoke validation through `make check-x86_64`.

Still blocking a desktop handoff:

1. Replace cooperative/shell-driven user execution with a real x86_64 scheduler
   that can keep multiple user processes alive and switch between them.
2. Add blocking waits, process sleep/wake, and timer-driven scheduling instead
   of only command-driven child reaping. The PIT tick source is present, but it
   is not yet driving preemptive context switches.
3. Expand process-owned address spaces beyond one code page and one stack page:
   mapped heap, guard pages, multi-segment ELF64 loads, and safe teardown.
4. Add a real persistent filesystem/storage path. The current VFS is useful for
   early userland and now has writable RAM-backed session storage, but a desktop
   needs storage-backed files, directories, and executable loading that survive
   reboot.
5. Expand the cursor-based input-event stream into blocking waits, per-window
   routing, and compositor-owned subscriptions.
6. Run an i386-vs-x86_64 parity pass before changing the default build.
7. Keep i386 only until x86_64 can do what the current i386 path does, then
   retire it deliberately.

## Guardrails

- Do not copy i386 files and rename them as if that made them 64-bit safe.
- Keep i386 bootable until x86_64 has verified parity.
- Keep architecture-specific code under architecture-specific directories.
