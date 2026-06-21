# ELF32 Loader Plan

## Purpose

Liam_OS Core currently executes flat userspace images. The next executable format target is ELF32 for i386.

The ELF32 loader should eventually allow Liam_OS to run separately built userspace binaries such as:

- `/bin/hello`
- `/bin/about`
- `/bin/cat`
- `/bin/sh`

## Current milestone

The current milestone only validates ELF32 headers. It does not execute ELF files yet.

Implemented:

- ELF32 header definitions
- ELF32 program-header definitions
- ELF magic validation
- 32-bit class validation
- little-endian validation
- executable type validation
- i386 machine validation
- basic program-header bounds validation
- shell command: `elf-check <path>`

## Next milestone

The next loader milestone should add real ELF loading:

1. Read ELF32 program headers.
2. Locate `PT_LOAD` segments.
3. Copy loadable segment bytes into user memory.
4. Zero memory where `p_memsz > p_filesz`.
5. Set the user entry point from `e_entry`.
6. Create a clean userspace stack.
7. Return a `user_image_t` that can be passed into the existing exec/process path.

## Not implemented yet

- ELF segment mapping
- ELF userspace stack setup
- ELF execution
- dynamic linking
- relocations
- shared libraries
- per-process page directories