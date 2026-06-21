# Liam_OS Desktop MVP

The first marketable Liam_OS Desktop release should be realistic and tightly scoped.

## MVP target

`Liam_OS Desktop 0.1 Developer Preview`

## Base recommendation

- Base OS: Ubuntu LTS or Debian.
- Desktop: KDE Plasma.
- Installer: Calamares or an equivalent proven installer.
- Package format: `.deb` if Ubuntu/Debian-based.
- Architecture: x86_64.

## Required MVP features

- Bootable live ISO.
- Installer that works in a VM and on at least one real test machine.
- Liam-branded boot, login, wallpaper, icon, cursor, and theme defaults.
- Liam Welcome first-run app.
- Profile selection: gamer, developer, creator, balanced.
- Clear update path through upstream repositories plus future Liam-specific packages.
- Basic documentation and website landing page.

## Explicit non-goals for MVP

- Custom kernel as the desktop runtime.
- Custom package manager.
- Custom compositor/window manager.
- GPU driver development.
- Full hardware certification.
- Commercial sale before update/support/legal foundations are ready.
