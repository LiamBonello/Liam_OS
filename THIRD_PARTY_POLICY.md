# Liam_OS Third-Party Acceleration Policy

This policy keeps Liam_OS development fast while preserving the ability to customize, brand, distribute, and sell the OS.

This is engineering guidance, not legal advice. Before commercial release, review the final dependency list with a qualified software licensing professional.

## Commercial Rule

Prefer third-party code only when it is under a permissive license that allows source and binary redistribution, modification, sublicensing or commercial sale, and does not require Liam_OS source code to be released.

Preferred license families:

- MIT
- BSD-2-Clause
- BSD-3-Clause
- 0BSD
- Apache-2.0
- zlib/libpng-style licenses

Avoid adding GPL, LGPL, AGPL, SSPL, BUSL, Commons Clause, source-available-only, non-commercial, or custom unclear licenses to the kernel, userspace runtime, installer, boot image, desktop shell, or bundled commercial product unless the obligation is intentionally accepted and documented first.

## Mandatory Import Checklist

Before importing any third-party component:

1. Record the component name, upstream URL, exact version or commit, and license.
2. Store the license text in `third_party/licenses/` or another tracked notice bundle.
3. Keep local modifications isolated under `third_party/` or behind a small adapter layer.
4. Add a short `README.md` beside the import explaining why it exists and how to update it.
5. Prefer source imports only after we have a minimal adapter design and a boot/test plan.
6. Do not mix GPL code into Liam_OS Core unless we decide to distribute the affected work under GPL-compatible terms.

## Recommended Acceleration Choices

### Bootloader: Limine

Recommendation: evaluate replacing or supplementing GRUB with Limine for the x86_64 path.

Reason: Limine is a modern bootloader with a permissive BSD-2-Clause license. This is simpler for a commercial custom OS than depending on GPL bootloader integration. It can also reduce x86_64 boot setup work over time.

Initial use: keep GRUB working, add Limine as an experimental alternate boot target, then switch only after QEMU boot parity.

### C Library / Userspace Runtime

Recommendation: use a staged approach.

Short term: keep the tiny assembly userspace while the syscall ABI is still moving.

Medium term: evaluate mlibc or Picolibc as the first real libc direction.

- mlibc is designed around porting to new operating systems through a syscall abstraction layer, which fits a custom kernel direction.
- Picolibc is smaller and embedded-oriented, useful if Liam_OS wants a compact C runtime before full POSIX-like support.
- musl is MIT-licensed and commercially friendly, but it targets the Linux syscall ABI, so it is not a drop-in libc for Liam_OS without a Linux-compatible syscall layer.
- Newlib is widely used in embedded systems, but it is a collection of multiple licenses, so it needs more careful license tracking than a single-license library.

### Filesystems

Recommendation: add FAT support first, then a native filesystem later.

- FatFs is a small FAT/exFAT module with a permissive custom license and a clean disk I/O boundary.
- littlefs is BSD-3-Clause and useful for small fail-safe storage, but it is less relevant for bootable PC-style disks than FAT at this stage.

Initial use: implement a block-device abstraction and port FatFs behind it for a read-only or read-mostly first milestone.

### USB

Recommendation: evaluate TinyUSB later, not before the kernel has stable interrupts, timers, memory allocation, and device discovery.

Reason: TinyUSB is MIT-licensed and portable, but host-controller support on PC hardware is still substantial kernel work. It is a good acceleration candidate once PCI and interrupt routing are more complete.

### Networking

Recommendation: evaluate lwIP after PCI/virtio or a basic NIC driver exists.

Reason: lwIP is BSD-style licensed and can provide IP, TCP, UDP, DHCP, and DNS faster than writing a network stack from scratch. It still needs a network driver and OS adapter layer.

### Graphics and Desktop UI

Recommendation: keep early desktop UI simple and permissively licensed.

- Dear ImGui is MIT-licensed and useful for internal tools, demos, and early graphical shells.
- stb-style single-file libraries or zlib/libpng-style libraries are useful for image loading/compression, subject to license tracking.
- FreeType can be used under the FreeType License, but it has attribution requirements; document the chosen license option explicitly.
- HarfBuzz is Old MIT licensed and useful later for serious text shaping.

Do not start with a full GPU stack. Build a framebuffer compositor first, then grow toward drivers.

### Compression / Archives

Recommendation: use zlib for compression when needed.

Reason: the zlib license explicitly permits commercial applications and redistribution with minimal obligations.

### TLS / Crypto

Recommendation: use Mbed TLS only when networking needs it.

Reason: Mbed TLS is dual licensed Apache-2.0 OR GPL-2.0-or-later. Choose and document Apache-2.0 for commercial Liam_OS use.

## Recommended Order

1. Keep the current custom kernel path as the product identity.
2. Add license tracking before importing dependencies.
3. Add Limine as an experimental x86_64 boot target while preserving GRUB.
4. Fix x86_64 process lifecycle: reserve PID 1 for shell, then reaping/cleanup.
5. Add a block-device abstraction.
6. Port FatFs read-only for FAT media and ISO/USB workflows.
7. Start a small libc direction with mlibc or Picolibc after syscalls stabilize.
8. Add virtio drivers in QEMU before broad physical hardware drivers.
9. Add framebuffer graphics and a small UI toolkit before a full desktop.

## Product Claiming Rules

Allowed claims when accurate:

- "Liam_OS Core is a custom operating system kernel."
- "Liam_OS uses selected permissive open-source components with attribution."
- "Liam_OS is customized and branded by Liam_OS."

Avoid claims if third-party code is included:

- "Every line is written from scratch."
- "No open-source code is used."
- "No license obligations apply."

The goal is not to avoid all obligations. The goal is to keep obligations predictable: attribution, license notices, and clear separation from Liam_OS-owned code.
