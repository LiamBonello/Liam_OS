# x86_64 Desktop Foundation Contracts

This file defines the stable kernel/userland contracts that must stay boring before Liam_OS moves deeper into desktop work.

## Build And Check Contract

- `make` builds the x86_64 ISO from the repository root.
- `make run` boots the x86_64 ISO in QEMU.
- `make check` runs the canonical x86_64 smoke test.
- i386 remains available only through explicit `legacy-i386` targets.

## Process Contract

- `/bin/init` is PID 1 and owns interactive shell behavior.
- User processes are spawned through the x86_64 exec/spawn syscall path.
- Child process completion is observed through `wait`.
- `ps` must report enough state to validate core services.

## Service Startup Contract

- Core services must be started once by init/session code, not opportunistically on every shell input.
- Service startup must report success or failure through serial-visible status text until a richer service manager exists.
- Service names should describe the subsystem they own, not test/demo behavior.

Current core services:

- `/bin/windowd-service`: keeps the window service reachable while the desktop session is still being split out.
- `/bin/sessiond`: smoke-tested desktop session entry point for status reporting, window presentation, input polling, and timer-paced execution.

## Framebuffer And Window Contract

- The framebuffer must expose width, height, pitch, bpp, RGB format, and mapped virtual address diagnostics.
- Window presentation must go through `x86_64_desktop_services_present_system_window`.
- `x86_64_desktop_services_present_demo_window` is a temporary compatibility alias and should be removed once all call sites are converted.
- The window present syscall returns `1` only after the system window has actually been drawn.

## Input Contract

- Keyboard readiness is reported through input status.
- Input events are read through the input-events syscall with bounded event counts.
- Empty event queues are valid and must not fail desktop readiness checks.

## Timer Contract

- Ticks must be monotonically observable.
- `sleep-ticks` must return after at least the requested number of ticks.
- Desktop/session code should use ticks for pacing instead of busy drawing loops.

## Storage Contract

Current storage is intentionally limited:

- read-only embedded VFS for system files and executables
- volatile `/tmp/session.txt` RAM-backed session file
- no persistent storage yet

Desktop work may rely on session storage for smoke/status checks, but user-facing desktop features should not imply persistence until a real writable filesystem or block-backed store exists.

## Desktop Session Contract

`/bin/sessiond` is wired into the x86_64 userland image and must remain covered by `make check`.

The next desktop milestone is to make `/bin/sessiond` a persistent session owner for:

- starting required services once
- reporting service readiness
- drawing the first desktop/session screen
- reading input events
- presenting frames at a timer-paced cadence
- exiting cleanly only when requested

Until `/bin/sessiond` owns the full session lifecycle, `deskcheck` remains the broad desktop readiness check.
