#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")/../core"
make check-tools
make clean
make
