#!/bin/sh
set -eu

LOG_PATH="${1:-build/x86_64/x86_64-smoke.log}"
ISO_PATH="${X86_64_ISO:-build/x86_64/liam_os_x86_64.iso}"
QEMU_BIN="${QEMU_X86_64:-qemu-system-x86_64}"
TIMEOUT_SECONDS="${LIAM_X86_64_SMOKE_TIMEOUT_SECONDS:-20}"
SUCCESS_MARKER="Desc/IST ok: 1"

mkdir -p "$(dirname "$LOG_PATH")"
rm -f "$LOG_PATH"

echo "Running x86_64 headless boot smoke test..."
echo "Serial log: $LOG_PATH"

"$QEMU_BIN" \
    -display none \
    -no-reboot \
    -no-shutdown \
    -monitor none \
    -serial "file:$LOG_PATH" \
    -boot d \
    -cdrom "$ISO_PATH" &
qemu_pid=$!

cleanup_qemu() {
    if kill -0 "$qemu_pid" >/dev/null 2>&1; then
        kill "$qemu_pid" >/dev/null 2>&1 || true
        wait "$qemu_pid" >/dev/null 2>&1 || true
    fi
}
trap cleanup_qemu EXIT INT TERM

elapsed=0
while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
    if [ -f "$LOG_PATH" ] && grep -F "$SUCCESS_MARKER" "$LOG_PATH" >/dev/null 2>&1; then
        cleanup_qemu
        trap - EXIT INT TERM
        break
    fi

    if ! kill -0 "$qemu_pid" >/dev/null 2>&1; then
        wait "$qemu_pid" || status=$?
        echo "QEMU exited before the success marker appeared."
        if [ "${status:-0}" -ne 0 ]; then
            echo "QEMU exit status: ${status:-0}"
        fi
        if [ -f "$LOG_PATH" ]; then
            cat "$LOG_PATH"
        fi
        exit 1
    fi

    sleep 1
    elapsed=$((elapsed + 1))
done

if [ "$elapsed" -ge "$TIMEOUT_SECONDS" ]; then
    echo "Timed out waiting for serial success marker: $SUCCESS_MARKER"
    if [ -f "$LOG_PATH" ]; then
        echo "--- serial log ---"
        cat "$LOG_PATH"
        echo "--- end serial log ---"
    fi
    exit 1
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
require_line "Stage: higher-half probe + descriptor"
require_line "Multiboot2: ok"
require_line "CPU CPUID available: 1"
require_line "CPU FPU: 1"
require_line "CPU MSR: 1"
require_line "CPU APIC: 1"
require_line "CPU SSE: 1"
require_line "CPU SSE2: 1"
require_line "CPU SYSCALL/SYSRET: 1"
require_line "CPU NX: 1"
require_line "CPU long mode: 1"
require_line "CPU baseline ok: 1"
require_line "IDT NMI IST: 2"
require_line "IDT NMI IST ok: 1"
require_line "IDT double fault IST: 1"
require_line "IDT double fault IST ok: 1"
require_line "IDT page fault IST: 3"
require_line "IDT page fault IST ok: 1"
require_line "IDT IST gates ok: 1"
require_line "PMM smoke free: 1"
require_line "Paging huge pages: 512"
require_line "VM identity PML4 index: 0"
require_line "VM kernel PML4 index: 511"
require_line "VM direct map PML4 index: 256"
require_line "VM planned regions: 3"
require_line "VM identity window ok: 1"
require_line "VM kernel canonical: 1"
require_line "VM direct map canonical: 1"
require_line "VM PML4 slots distinct: 1"
require_line "VM plan ok: 1"
require_line "Paging builder PML4 entries: 3"
require_line "Paging builder identity huge pages: 512"
require_line "Paging builder direct huge pages: 512"
require_line "Paging builder identity ok: 1"
require_line "Paging builder direct map ok: 1"
require_line "Paging builder kernel ok: 1"
require_line "Paging builder tables aligned: 1"
require_line "Paging builder ok: 1"
require_line "Paging activation builder ready: 1"
require_line "Paging activation CR3 changed: 1"
require_line "Paging activation active matches builder: 1"
require_line "Paging activation ok: 1"
require_line "Paging probe activation ready: 1"
require_line "Paging probe identity ok: 1"
require_line "Paging probe direct map ok: 1"
require_line "Paging probe kernel alias ok: 1"
require_line "Paging probes ok: 1"
require_line "Higher-half probe activation ready: 1"
require_line "Higher-half probe alias ready: 1"
require_line "Higher-half probe low ok: 1"
require_line "Higher-half probe high ok: 1"
require_line "Higher-half probe ok: 1"
require_line "GDT/TSS loaded ok: 1"
require_line "Desc/IST ok: 1"

echo "x86_64 headless boot smoke test passed."
