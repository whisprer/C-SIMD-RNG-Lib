#!/usr/bin/env bash
# Absolute, merciless CMake/Ninja/VS build artifact purge for this tree.
# Usage:
#   ./tools/ua_purge_cmake.sh         # destructive
#   ./tools/ua_purge_cmake.sh --dry   # print what would be deleted

set -euo pipefail

DRY=0
[[ "${1:-}" == "--dry" || "${1:-}" == "--dry-run" ]] && DRY=1

say() { printf '[purge] %s\n' "$*"; }
rm_f() { if [[ $DRY -eq 1 ]]; then say "DEL  (dry) $1"; else rm -f -- "$1" 2>/dev/null || true; say "DEL  $1"; fi; }
rm_rf() { if [[ $DRY -eq 1 ]]; then say "RMDIR(dry) $1"; else rm -rf -- "$1" 2>/dev/null || true; say "RMDIR $1"; fi; }

ROOT="$(pwd)"
say "root: $ROOT"
say "mode: $([[ $DRY -eq 1 ]] && echo DRY-RUN || echo DESTRUCTIVE)"

# 1) Known build directories (common IDEs & CMake conventions)
DIR_PATTERNS=(
  "build" "build-*" "cmake-build-*" "out/build" "out/build-*"
  ".vs" "_deps" "_cmake_test_*" "CMakeFiles" "CMakeScripts"
  ".cache" ".cmake" ".ninja" "Debug" "Release" "RelWithDebInfo" "MinSizeRel"
)

# 2) Standalone files to remove
FILE_PATTERNS=(
  "CMakeCache.txt" "CMakeUserPresets.json" "CMakePresets.json"
  "Makefile" "build.ninja" "rules.ninja" ".ninja_deps" ".ninja_log"
  "compile_commands.json" "CTestTestfile.cmake" "DartConfiguration.tcl"
  "*.sln" "*.vcxproj" "*.vcxproj.*" "*.vcproj" "*.vcproj.*" "*.code-workspace"
  "*.obj" "*.o" "*.a" "*.lib" "*.exp" "*.ilk" "*.pdb" "*.dll" "*.exe" "*.out" "*.manifest"
  "*.cache" "*.dir"
)

# Delete dirs
for pat in "${DIR_PATTERNS[@]}"; do
  while IFS= read -r -d '' d; do rm_rf "$d"; done < <(find "$ROOT" -type d -name "$pat" -print0 2>/dev/null || true)
done

# Delete files
for pat in "${FILE_PATTERNS[@]}"; do
  while IFS= read -r -d '' f; do rm_f "$f"; done < <(find "$ROOT" -type f -name "$pat" -print0 2>/dev/null || true)
done

say "purge complete."
