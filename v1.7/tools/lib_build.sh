#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
INSTALL_DIR="${ROOT}/dist"

echo "[ua] root:    $ROOT"
echo "[ua] build:   $BUILD"
echo "[ua] install: $INSTALL_DIR"

rm -rf "$BUILD" "$INSTALL_DIR"
mkdir -p "$BUILD" "$INSTALL_DIR"

cmake -S "$ROOT" -B "$BUILD" -G "Ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  -DUA_BUILD_BENCH=ON \
  -DUA_ENABLE_AVX2=ON \
  -DUA_ENABLE_AVX512=ON \
  -DUA_ENABLE_LTO=ON \
  -DUA_BUILD_STATIC=ON \
  -DUA_BUILD_SHARED=ON

cmake --build "$BUILD" -j
cmake --install "$BUILD" --prefix "$INSTALL_DIR"

echo "[ua] installed files:"
find "$INSTALL_DIR" -maxdepth 3 -type f | sed "s|$INSTALL_DIR/||" | sort
