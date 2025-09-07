#!/usr/bin/env bash
set -euo pipefail
set -x

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
DIST="${ROOT}/dist"

rm -rf "$BUILD" "$DIST"
mkdir -p "$BUILD"

cmake -S "$ROOT" -B "$BUILD" -G "Ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  -DUA_BUILD_BENCH=ON \
  -DUA_ENABLE_AVX2=ON \
  -DUA_ENABLE_AVX512=ON \
  -DUA_BUILD_STATIC=ON \
  -DUA_BUILD_SHARED=ON

cmake --build "$BUILD" -v -j

# try install; if it fails, still stage artifacts so you have libs
if cmake --install "$BUILD" --prefix "$DIST" --verbose; then
  echo "[ua] install succeeded"
else
  echo "[ua] install failed; staging from build tree instead"
  "$ROOT/tools/stage_dist.sh"
fi

# show result
find "$DIST" -maxdepth 3 -type f -print | sed "s|^$DIST/||" | sort || true

# Bench availability (optional)
if [ -f "$BUILD/ua_rng_bench.exe" ]; then
  echo "[ua] run bench once"
  "$BUILD/ua_rng_bench.exe" || true
fi
