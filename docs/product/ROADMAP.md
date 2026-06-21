# Liam_OS Roadmap

## Track A: Liam_OS Core

### Milestone A1: Stabilize current i386 kernel

- Keep current GRUB Multiboot i386 path buildable.
- Keep diagnostics and shell commands working.
- Document the syscall ABI.
- Add build verification scripts.
- Avoid mixing x86_64 assumptions into i386 code.

### Milestone A2: Userspace maturity

- Implement ELF32 loading.
- Add proper user stack setup.
- Add file syscalls such as `open`, `read`, `close`, and `stat`.
- Move `/sbin/init` from embedded image assumptions toward real VFS-backed executable loading.

### Milestone A3: Process isolation

- Introduce per-process address spaces.
- Add safe kernel/user copy helpers.
- Separate kernel and user memory permissions.
- Define process lifecycle semantics.

### Milestone A4: Storage and persistence

- Add block-device abstraction.
- Add ATA first, AHCI later.
- Add simple read-only filesystem support before read/write support.

### Milestone A5: Graphics/input foundation

- Add framebuffer boot path.
- Add pixel drawing and font rendering.
- Add mouse input.
- Add event queues.
- Defer GPU acceleration until a much later stage.

### Milestone A6: x86_64 port

- Add `kernel/arch/x86_64/` as a separate architecture.
- Implement long mode boot.
- Implement x86_64 paging and syscall entry.
- Do not overwrite the working i386 path prematurely.

## Track B: Liam_OS Desktop

### Milestone B1: Distribution prototype

- Choose Ubuntu LTS or Debian base.
- Choose KDE Plasma as the initial desktop environment.
- Create a bootable live ISO prototype.
- Brand boot, login, wallpaper, panel layout, and default theme.

### Milestone B2: Liam first-run experience

- Build Liam Welcome.
- Let users choose gamer, developer, creator, or balanced profiles.
- Configure theme, accent color, animation intensity, and app bundles.

### Milestone B3: Profile managers

- Gaming profile: Steam setup guidance, Proton tools, MangoHud, GameMode, Lutris/Heroic installers.
- Developer profile: Git, VS Code, Docker, Node, Python, Rust, Go, terminal profiles.
- Creator profile: Blender, Krita, GIMP, Inkscape, font tools, color tools, recording tools.

### Milestone B4: Commercial readiness

- Define license obligations.
- Avoid bundling proprietary apps unless redistribution rights are confirmed.
- Add privacy policy and update policy.
- Provide support lifecycle documentation.
