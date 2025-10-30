#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
DIST="${ROOT}/dist"

echo "[ua] STAGE from $BUILD -> $DIST"

rm -rf "$DIST"
mkdir -p "$DIST/bin" "$DIST/lib" "$DIST/include" "$DIST/lib/cmake/ua_rng"

# headers
if [ -d "$ROOT/include" ]; then
  cp -R "$ROOT/include/ua" "$DIST/include/"
fi

# libs from build tree (MinGW/Ninja default locations)
# static
find "$BUILD" -maxdepth 2 -type f -name "libua_rng.a" -exec cp -v {} "$DIST/lib/" \; || true
# import lib (for the DLL)
find "$BUILD" -maxdepth 2 -type f -name "libua_rng.dll.a" -exec cp -v {} "$DIST/lib/" \; || true
# dll
find "$BUILD" -maxdepth 2 -type f -name "ua_rng.dll" -exec cp -v {} "$DIST/bin/" \; || true
# pkg cmake files (if generated)
find "$BUILD" -maxdepth 2 -type f -name "ua_rngTargets.cmake" -exec cp -v {} "$DIST/lib/cmake/ua_rng/" \; || true
find "$BUILD" -maxdepth 2 -type f -name "ua_rngConfig.cmake" -exec cp -v {} "$DIST/lib/cmake/ua_rng/" \; || true
find "$BUILD" -maxdepth 2 -type f -name "ua_rngConfigVersion.cmake" -exec cp -v {} "$DIST/lib/cmake/ua_rng/" \; || true

echo "[ua] staged tree:"
( cd "$DIST" && find . -type f | sort )
