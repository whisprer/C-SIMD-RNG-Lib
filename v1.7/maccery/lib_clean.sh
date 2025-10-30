#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
echo "[ua] cleaning $ROOT/build and $ROOT/dist"
rm -rf "$ROOT/build" "$ROOT/dist"
