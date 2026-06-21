# Liam_OS Core i386 Architecture Assumptions

This note captures the current 32-bit assumptions that must stay explicit while Liam_OS begins a staged x86_64 migration. The existing i386 path remains the bootable baseline until the x86_64 path has its own proven boot, interrupt, paging, and userspace story.

## Boot and CPU mode

- The current kernel boots through GRUB Multiboot into 32-bit protected mode.
- The active architecture directory is `core/kernel/arch/i386/`.
- `core/Makefile` hardcodes `ARCH := i386` and builds `kernel/arch/i386` assembly and C sources.
- The current linker path targets an i386 ELF kernel through `ld.lld -m elf_i386`.
- Early entry, GDT loading, IDT stubs, paging activation, IRQ stubs, context switching, and ring-3 return paths are all i386-specific.

## Addressing and paging

- Kernel code uses 32-bit integer addresses through `uint32_t` across PMM, VMM, heap, process, loader, and syscall paths.
- Paging is 32-bit page-directory/page-table based, with 1024 directory entries and 1024 table entries.
- Current memory layout constants assume 32-bit virtual addresses:
  - identity range: `0x00000000` through `0x07FFFFFF`
  - user code page: `0x10000000`
  - user stack page: `0x10001000`
  - higher-half start: `0xC0000000`
  - kernel heap: `0xC1000000` through `0xC1FFFFFF`
- Physical memory allocation currently selects one usable Multiboot memory-map region and stores page addresses as `uint32_t`.

## Ring 3 and syscalls

- The userspace model is currently a single shared one-page code mapping and one shared one-page stack mapping.
- Flat binaries are copied into `RING3_USER_CODE_VIRTUAL` and entered through an i386 `iretd` transition.
- Syscalls use `int 0x80` and pass arguments through 32-bit general-purpose registers.
- `ring3_entry.asm` owns the i386 user transition, syscall stub, and kernel return path only. It must not regain embedded user programs.

## Processes and user images

- Process records hold 32-bit entry and stack pointers.
- `LIAM_SYSCALL_EXEC` currently creates READY processes, but child processes reuse the same early ring-3 mapping model when run.
- ELF32 support is intentionally limited to i386 executable files with one `PT_LOAD` segment at the current ring-3 user code virtual address.
- Userland flat binaries live under `core/userland/flat/<name>/<name>.asm` and are embedded through `core/kernel/userland/userland_images.asm`.

## Migration guardrails

- Keep the existing `core/kernel/arch/i386/` path intact while adding `core/kernel/arch/x86_64/`.
- Do not rename i386 files or constants to x86_64 unless the implementation is actually 64-bit safe.
- Introduce x86_64 build targets separately from the i386 default target.
- Move shared architecture contracts behind small headers only after both architectures need them.
- Port kernel boot, long mode, interrupt handling, and paging before attempting a 64-bit userspace model.
