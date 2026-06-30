#!/usr/bin/env sh
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/dist"
mkdir -p "$OUT"
cd "$ROOT"

find . \
  -path './.git' -prune -o \
  -path './core/build' -prune -o \
  -path './dist' -prune -o \
  -type f -print | sort > SOURCE_MANIFEST.txt

if command -v git >/dev/null 2>&1 && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  TMP="$(mktemp -d)"
  trap 'rm -rf "$TMP"' EXIT
  mkdir -p "$TMP/Liam_OS"
  { git ls-files -z; git ls-files --others --exclude-standard -z; } |
    tar --null -T - -cf - |
    (cd "$TMP/Liam_OS" && tar -xf -)
  (cd "$TMP" && zip -qr "$OUT/Liam_OS_source.zip" Liam_OS)
else
  TMP="$(mktemp -d)"
  trap 'rm -rf "$TMP"' EXIT
  rsync -a --exclude='.git' --exclude='core/build' --exclude='dist' "$ROOT/" "$TMP/Liam_OS/"
  (cd "$TMP" && zip -qr "$OUT/Liam_OS_source.zip" Liam_OS)
fi
printf '%s\n' "$OUT/Liam_OS_source.zip"
