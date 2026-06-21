#!/usr/bin/env sh
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/dist"
mkdir -p "$OUT"
cd "$ROOT"
if command -v git >/dev/null 2>&1 && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git archive --format=zip --output="$OUT/Liam_OS_source.zip" HEAD
else
  TMP="$(mktemp -d)"
  trap 'rm -rf "$TMP"' EXIT
  rsync -a --exclude='.git' --exclude='core/build' --exclude='dist' "$ROOT/" "$TMP/Liam_OS/"
  (cd "$TMP" && zip -qr "$OUT/Liam_OS_source.zip" Liam_OS)
fi
printf '%s\n' "$OUT/Liam_OS_source.zip"
