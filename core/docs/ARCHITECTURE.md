# Liam OS Architecture

Liam OS is currently a freestanding i386 kernel with a protected-mode boot path, kernel console, interrupt handling, memory management, task scheduling primitives, a syscall dispatcher, and an initial userspace execution path.

The current implementation is intentionally structured as a real OS foundation rather than a desktop mockup. Desktop, automation, gaming, and developer-facing features should be built above stable process, executable-loading, filesystem, input, and graphics subsystems.

## Current Architecture

```txt
GRUB Multiboot
  -> i386 protected-mode kernel entry
  -> GDT / TSS / IDT / PIC
  -> physical memory manager
  -> paging and virtual memory manager
  -> heap
  -> timer / keyboard / VGA console
  -> scheduler primitives
  -> process manager
  -> syscall dispatcher
  -> userspace image registry
  -> exec subsystem
  -> kernel shell
```

## Architecture Layout

```txt
kernel/
  arch/i386/       CPU, GDT, IDT, paging, TSS, ring-3 transition code
  core/            kernel services, process manager, exec, syscalls, shell
  drivers/         VGA, timer, keyboard
  memory/          PMM, VMM, heap, memory layout
boot/              GRUB configuration
docs/              architecture and release notes
```

The active architecture is `i386`. The previous misleading `x86_64` directory name was corrected because the build target is currently 32-bit:

```make
-target i386-elf
-f elf32
-m elf_i386
qemu-system-i386
```

A future x86_64 port should be added as a separate architecture implementation under `kernel/arch/x86_64/`, not by mixing 64-bit assumptions into the current i386 code.

## Userspace Execution

Userspace execution now flows through production-oriented subsystems:

```txt
exec_spawn(path)
  -> user_image_resolve(path)
  -> process_create(...)
  -> process_run(...)
  -> ring3_run_user_image(...)
  -> syscall_dispatch(...)
```

The first registered userspace image is:

```txt
/sbin/init
```

At this stage `/sbin/init` is an embedded bootstrap image because there is not yet a filesystem or ELF loader. It is intentionally exposed through the same exec/image/process interface that later filesystem-backed binaries will use. The next production step is to replace embedded image resolution with a VFS-backed loader, then an ELF loader.

## Syscall ABI

The syscall dispatcher is the kernel/user boundary. The current syscall numbers are:

```txt
1  exit(code)
2  write(buffer, length)
3  get_ticks()
4  yield()
5  get_pid()
```

The current syscall mechanism uses `int 0x80` on i386. A future x86_64 port can keep the ABI numbers while replacing the CPU-level entry mechanism with `syscall/sysret` after long mode is implemented.

## Shell Commands for Core Execution

Production-facing execution commands:

```txt
images       List registered userspace images
exec-status  Show exec and image registry state
ps           Show process table
exec <path>  Start a userspace image
init         Start /sbin/init
syscalls     Show syscall dispatcher statistics
```

Low-level hardware/kernel diagnostic commands remain available because this OS is still under kernel bring-up. They should eventually be hidden behind a debug build or moved into a dedicated diagnostic shell.

## Next Required Subsystems

The next production milestones are:

```txt
1. VFS abstraction
2. Initramfs or memory-backed filesystem
3. Flat binary loader
4. ELF loader
5. Filesystem-backed /sbin/init
6. Userspace shell
7. Storage driver
8. Framebuffer graphics
9. Mouse/input event queue
10. Window manager and desktop shell
```

## VFS and initramfs

Liam OS now mounts an immutable initramfs-backed VFS during boot. The VFS is the kernel's canonical namespace for early files, system metadata, devices, and executable image discovery.

Initial namespace:

```txt
/
├─ sbin/
│  └─ init
├─ etc/
│  └─ os-release
└─ dev/
   └─ console
```

The current `/sbin/init` userspace image is still backed by the embedded ring-3 image executor, but the exec subsystem now validates the path through VFS resolution. This prepares the project for replacing the embedded image with a flat binary loader and then an ELF loader.

## Userspace loading model

The current userspace execution path is now VFS-backed:

```text
exec_spawn(path)
  -> user_image_resolve(path)
  -> vfs_lookup(path)
  -> flat_binary_load_from_node(node)
  -> ring3_load_user_image(bytes, size)
  -> process_create(...)
  -> process_run(...)
  -> ring3_run_user_image(...)
```

`/sbin/init` is stored in the initramfs as flat executable bytes exported from the architecture-specific ring-3 image section. The loader copies those bytes into the mapped user code page before execution. This is still a flat binary format, not ELF. The loader boundary is intentionally separate so ELF can be added later without changing exec/process APIs.

The kernel is not desktop-ready yet. Required remaining kernel milestones before a real desktop are process address spaces, file syscalls, executable format loading beyond flat binaries, graphics framebuffer support, mouse/input event routing, and IPC/event delivery.
