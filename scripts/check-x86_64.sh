#!/usr/bin/env sh
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CORE="$ROOT/core"
LOG="${TMPDIR:-/tmp}/liam_os_x86_64_smoke.log"

cd "$CORE"
make clean
make x86_64-iso

rm -f "$LOG"

printf '%s\n' 'Running x86_64 QEMU smoke test...'
printf 'Smoke log: %s\n' "$LOG"

set +e
timeout 45s sh -c '
  {
    sleep 4
    printf "version\n"
    sleep 1
    printf "pid\n"
    sleep 1
    printf "ps\n"
    sleep 1
    printf "exec /bin/windowd\n"
    sleep 2
    printf "wait\n"
    sleep 2
    printf "deskcheck\n"
    sleep 10
    printf "exit\n"
    sleep 1
  } | qemu-system-x86_64 -display none -monitor none -serial stdio -boot d -cdrom build/x86_64/liam_os_x86_64.iso
' >"$LOG" 2>&1
status=$?
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
  cat "$LOG"
  exit "$status"
fi

require_marker() {
  marker="$1"
  if ! grep -F "$marker" "$LOG" >/dev/null 2>&1; then
    printf 'Missing x86_64 smoke marker: %s\n' "$marker"
    printf '%s\n' '--- serial log ---'
    cat "$LOG"
    exit 1
  fi
}

require_marker "Liam_OS x86_64 shell online"
require_marker "Framebuffer found: 1"
require_marker "Framebuffer width: 1024"
require_marker "Framebuffer height: 768"
require_marker "Framebuffer bpp: 32"
require_marker "Framebuffer type: 1"
require_marker "Framebuffer RGB format ok: 1"
require_marker "Framebuffer virtual address: 0xFFFF900000000000"
require_marker "Framebuffer huge pages: 2"
require_marker "Framebuffer mapping ready: 1"
require_marker "Framebuffer surface ready: 1"
require_marker "Framebuffer smoke ok: 1"
require_marker "Desktop scheduler ticks ready: 1"
require_marker "Desktop input ready: 1"
require_marker "Desktop input events ready: 1"
require_marker "Desktop input event capacity: 64"
require_marker "Desktop storage readonly VFS ready: 1"
require_marker "Desktop session storage ready: 1"
require_marker "Desktop persistent storage ready: 0"
require_marker "Desktop graphics ready: 1"
require_marker "Desktop window service ready: 1"
require_marker "Desktop services smoke ok: 1"
require_marker "Liam_OS Core x86_64 0.8.69-dev"
require_marker "pid: 1"
require_marker "PID STATE MODE NAME"
require_marker "created 1 user-created 0 user-exited 0 user-reaped 0"
require_marker "Liam_OS window service"
require_marker "storage-session 1"
require_marker "window-service-ready 1"
require_marker "window-present ok"
require_marker "wait: pid 2"
require_marker "wait: exit 0"
require_marker "Liam_OS desktop readiness check"
require_marker "Liam_OS timer service"
require_marker "sleep-ticks ok"
require_marker "Liam_OS input service"
require_marker "keyboard-ready 1"
require_marker "event-capacity 64"
require_marker "event-oldest 0"
require_marker "event-latest 0"
require_marker "Liam_OS input event service"
require_marker "events read 0"
require_marker "Liam_OS storage service"
require_marker "session storage ok"
require_marker "storage-write ok"
require_marker "Liam_OS x86_64 shell exited"

printf '%s\n' 'x86_64 smoke test passed'
printf 'x86_64 smoke log: %s\n' "$LOG"
