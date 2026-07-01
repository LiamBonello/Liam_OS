#!/usr/bin/env sh
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CORE="$ROOT/core"
LOG="${TMPDIR:-/tmp}/liam_os_x86_64_smoke.log"
DISK="${TMPDIR:-/tmp}/liam_os_x86_64_smoke_disk.img"

cd "$CORE"
make clean
make x86_64-iso

rm -f "$LOG" "$DISK"
truncate -s 64M "$DISK"
export DISK

printf '%s\n' 'Running x86_64 QEMU smoke test...'
printf 'Smoke log: %s\n' "$LOG"

set +e
timeout 150s sh -c '
  {
    sleep 4
    printf "version\n"
    sleep 1
    printf "pid\n"
    sleep 1
    printf "ps\n"
    sleep 1
    printf "exec /bin/datatest\n"
    sleep 2
    printf "wait\n"
    sleep 2
    printf "exec /bin/windowd\n"
    sleep 3
    printf "wait\n"
    sleep 2
    printf "deskcheck\n"
    sleep 10
    printf "exec /bin/storaged\n"
    sleep 6
    printf "wait\n"
    sleep 4
    printf "exec /bin/terminal\n"
    sleep 7
    printf "/bin/terminal\n"
    sleep 7
    printf "wait\n"
    sleep 5
    printf "exec /bin/file-manager\n"
    sleep 7
    printf "/bin/file-manager\n"
    sleep 7
    printf "wait\n"
    sleep 5
    printf "exec /bin/settings\n"
    sleep 6
    printf "wait\n"
    sleep 5
    printf "exec /bin/system-monitor\n"
    sleep 6
    printf "wait\n"
    sleep 5
    printf "exec /bin/text-editor\n"
    sleep 6
    printf "wait\n"
    sleep 5
    printf "exec /bin/sessiond\n"
    sleep 8
  } | qemu-system-x86_64 \
    -display none \
    -monitor none \
    -serial stdio \
    -boot d \
    -cdrom build/x86_64/liam_os_x86_64.iso \
    -device ich9-ahci,id=ahci \
    -drive if=none,id=liamdisk,file="$DISK",format=raw \
    -device ide-hd,drive=liamdisk,bus=ahci.0
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
require_marker "x86_64 PCI bus discovery online"
require_marker "PCI config I/O ready: 1"
require_marker "PCI devices found: "
require_marker "PCI storage controllers: "
require_marker "PCI AHCI controllers: 1"
require_marker "PCI NVMe controllers: "
require_marker "x86_64 storage hardware inventory online"
require_marker "Storage hardware initialized: 1"
require_marker "Storage PCI ready: 1"
require_marker "Storage AHCI controllers: 1"
require_marker "Storage AHCI controller found: 1"
require_marker "Storage AHCI BAR5 raw: 0x"
require_marker "Storage AHCI MMIO base: 0x"
require_marker "Storage AHCI MMIO virtual: 0x"
require_marker "Storage AHCI MMIO BAR ready: 1"
require_marker "Storage AHCI MMIO mapped: 1"
require_marker "x86_64 AHCI probe online"
require_marker "AHCI initialized: 1"
require_marker "AHCI controller found: 1"
require_marker "AHCI BAR ready: 1"
require_marker "AHCI MMIO mapped: 1"
require_marker "AHCI HBA probe safe: 1"
require_marker "AHCI HBA registers read: 1"
require_marker "AHCI ports implemented: "
require_marker "AHCI command slots: "
require_marker "AHCI ports scanned: "
require_marker "AHCI device ports: 0x"
require_marker "AHCI SATA device ports: 0x"
require_marker "AHCI ATAPI device ports: 0x"
require_marker "AHCI first device port: "
require_marker "AHCI first device signature: 0x"
require_marker "AHCI first device status: 0x"
require_marker "AHCI command buffers allocated: "
require_marker "AHCI command list page: 0x"
require_marker "AHCI received FIS page: 0x"
require_marker "AHCI command table page: 0x"
require_marker "AHCI DMA buffer page: 0x"
require_marker "AHCI command port programmed: "
require_marker "AHCI command list bound: "
require_marker "AHCI received FIS bound: "
require_marker "AHCI command engine stopped: "
require_marker "AHCI command engine started: "
require_marker "AHCI command engine ready: "
require_marker "AHCI read slot ready: 1"
require_marker "AHCI read command prepared: 1"
require_marker "AHCI read command issued: 1"
require_marker "AHCI read command completed: 1"
require_marker "AHCI read sector ready: 1"
require_marker "AHCI read sector zeroed: 1"
require_marker "AHCI read error: 0"
require_marker "AHCI driver ready: 1"
require_marker "Storage block driver ready: 0"
require_marker "Storage persistent ready: 0"
require_marker "Desktop graphics ready: 1"
require_marker "Desktop window service ready: 1"
require_marker "Desktop services smoke ok: 1"
require_marker "Liam_OS Core x86_64 0.8.69-dev"
require_marker "pid: 1"
require_marker "PID STATE MODE NAME"
require_marker "init: service pid "
require_marker "/bin/windowd-service"
require_marker "created 2 user-created 1 user-exited 0 user-reaped 0"
require_marker "selfcheck: writable data segment ok"
require_marker "wait: pid "
require_marker "wait: exit 0"
require_marker "kernel-sched "
require_marker "user-sched "
require_marker "kernel-cursor "
require_marker "user-cursor "
require_marker "Liam_OS window service"
require_marker "storage-session 1"
require_marker "window-service-ready 1"
require_marker "window-present ok"
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
require_marker "storage-mkdir ok"
require_marker "storage-create ok"
require_marker "ramfs userland ok"
require_marker "storage-read ok"
require_marker "storage-delete ok"
require_marker "storage-ramfs ok"
require_marker "Liam_OS Terminal"
require_marker "terminal: ready"
require_marker "Liam_OS File Manager"
require_marker "file-manager: ready"
require_marker "Liam_OS Settings"
require_marker "settings: ready"
require_marker "Liam_OS System Monitor"
require_marker "system-monitor: ready"
require_marker "Liam_OS Text Editor"
require_marker "text-editor: ready"
require_marker "exec /bin/sessiond"
require_marker "Liam_OS desktop session"
require_marker "sessiond: system window presented"
require_marker "sessiond: ready"

printf '%s\n' 'x86_64 smoke test passed'
printf 'x86_64 smoke log: %s\n' "$LOG"
