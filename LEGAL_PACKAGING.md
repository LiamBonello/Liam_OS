# Legal Packaging Notes

These notes are a release gate, not legal advice.

## Required Decisions Before Selling

- Choose and document the project license for source, binaries, branding, and documentation.
- Decide whether commercial builds use an EULA, an open-source license, or a dual-license model.
- Add copyright notices for project-owned files.
- Add third-party notices for every bundled dependency, toolchain component, bootloader component, font, asset, driver, and library.
- Confirm whether any firmware, driver, codec, cryptography, trademark, patent, export-control, privacy, or telemetry obligations apply.
- Define warranty, liability, refund, update-support, and end-of-life terms.

## Release Artifacts

Every public release should include:

- Source archive.
- Bootable image or installer image.
- Checksums.
- Signature metadata once signing is implemented.
- Changelog.
- License/EULA.
- Third-party notices.
- Security contact and supported-version policy.

Do not market Liam_OS as production-ready until preemptive multitasking, process isolation, persistent storage, installer/update/signing, driver policy, and security response workflows are complete and tested.

