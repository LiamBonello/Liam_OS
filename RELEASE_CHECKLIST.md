# Liam_OS Release Checklist

Use this checklist before publishing any public build, installer image, source archive, or paid release.

## Build and Smoke Tests

- Run `make check-x86_64` and keep the serial smoke log for the release record.
- Run `./scripts/check-core.sh` for the i386/core compatibility path.
- Run `git diff --check` before packaging.
- Run `./scripts/package-source.sh` and verify the archive contains the expected source files.
- Boot-test the release ISO in QEMU with a clean virtual disk image.

## Kernel and Desktop Readiness

- Verify timer, keyboard, framebuffer, syscall, VFS, process, and desktop-service diagnostics.
- Verify input-event sequence counters and service snapshots.
- Verify no panic path or diagnostic self-test flag is enabled in the release boot config.
- Confirm known blockers are documented in `core/kernel/arch/x86_64/README.md`.

## Installer, Updates, and Signing

- Produce a release manifest with image name, version, build date, source archive name, and checksums.
- Generate SHA-256 checksums for every distributed artifact.
- Sign release artifacts after the signing key and certificate process is finalized.
- Verify installer or update images on an empty disk and on an upgrade path before publishing.
- Keep signing keys outside the repository and outside build artifacts.

## Security and Legal

- Review `SECURITY.md` for supported versions and reporting contact details.
- Review `LEGAL_PACKAGING.md` before any commercial distribution.
- Confirm the project license, third-party notices, warranty terms, export controls, trademark usage, and privacy obligations.
- Confirm bundled toolchains, bootloaders, fonts, firmware, drivers, and libraries are redistributable under their licenses.

