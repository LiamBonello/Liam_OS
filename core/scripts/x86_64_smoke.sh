#!/bin/sh
set -eu

LOG_PATH="${1:-build/x86_64/x86_64-smoke.log}"
ISO_PATH="${X86_64_ISO:-build/x86_64/liam_os_x86_64.iso}"
QEMU_BIN="${QEMU_X86_64:-qemu-system-x86_64}"
TIMEOUT_SECONDS="${LIAM_X86_64_SMOKE_TIMEOUT_SECONDS:-20}"

mkdir -p "$(dirname "$LOG_PATH")"
rm -f "$LOG_PATH"

echo "Running x86_64 headless boot smoke test..."
echo "Serial log: $LOG_PATH"

set +e
timeout "$TIMEOUT_SECONDS" "$QEMU_BIN" \
    -display none \
    -no-reboot \
    -no-shutdown \
    -monitor none \
    -serial "file:$LOG_PATH" \
    -boot d \
    -cdrom "$ISO_PATH"
status=$?
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
    echo "QEMU exited unexpectedly with status $status"
    if [ -f "$LOG_PATH" ]; then
        cat "$LOG_PATH"
    fi
    exit "$status"
fi

require_line() {
    expected="$1"
    if ! grep -F "$expected" "$LOG_PATH" >/dev/null 2>&1; then
        echo "Missing expected serial line: $expected"
        echo "--- serial log ---"
        cat "$LOG_PATH"
        echo "--- end serial log ---"
        exit 1
    fi
}

require_line "Liam_OS x86_64 kernel diagnostics"
require_line "Stage: IST gates + descriptor + PMM"
require_line "Multiboot2: ok"
require_line "IDT NMI IST: 2"
require_line "IDT NMI IST ok: 1"
require_line "IDT double fault IST: 1"
require_line "IDT double fault IST ok: 1"
require_line "IDT page fault IST: 3"
require_line "IDT page fault IST ok: 1"
require_line "IDT IST gates ok: 1"
require_line "PMM smoke free: 1"
require_line "Paging huge pages: 512"
require_line "GDT/TSS loaded ok: 1"
require_line "Desc/IST ok: 1"

echo "x86_64 headless boot smoke test passed."
