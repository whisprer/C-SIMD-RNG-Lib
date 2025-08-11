# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# \# Universal Architecture PRNG std. Replacement C++\_Lib.
# 
# \*\*A high-performance, cross-platform random number generation library with SIMD and GPU acceleration.\*\*
# 
# ---
# 
# \## Overview
# 
# `universal\_rng\_lib` is a fast, flexible RNG library written in modern C++. It supports a range of algorithms including \*\*Xoroshiro128++\*\* and \*\*WyRand\*\*, with runtime autodetection of the best CPU vectorization (SSE2, AVX2, AVX-512, NEON) and optional OpenCL GPU support.
# 
# It significantly outperforms the C++ standard library RNGs and can replace them in scientific simulations, games, real-time systems, and more.
# 
# ---
# 
# \## Features
# 
# \- ✅ Multiple PRNGs: `Xoroshiro128++`, `WyRand`
# \- ✅ SIMD Acceleration: SSE2, AVX2, AVX-512, NEON (auto-detect at runtime)
# \- ✅ OpenCL GPU support (optional)
# \- ✅ Scalar fallback for universal compatibility
# \- ✅ Batch generation for improved throughput
# \- ✅ Support for 16–1024 bit generation
# \- ✅ Cross-platform: Windows (MSVC, MinGW), Linux
# \- ✅ MIT Licensed
# 
# ---
# 
# \## Quick Start
# 
# \### Requirements
# 
# \- C++17-compatible compiler
# \- CMake 3.15+
# \- Ninja (recommended)
# \- OpenCL SDK (optional)
# 
# \### Build Instructions
# 
# \#### Linux/Mac (bash)
# ```bash
# git clone https://github.com/YOUR\_USERNAME/universal\_rng\_lib.git
# cd universal\_rng\_lib
# mkdir build \&\& cd build
# cmake .. -G Ninja -DCMAKE\_BUILD\_TYPE=Release
# cmake --build . --parallel
# ./rng\_selftest
# Windows (MSYS2 MinGW64 shell)
#
# bash
# git clone https://github.com/YOUR\_USERNAME/universal\_rng\_lib.git
# cd universal\_rng\_lib
# mkdir build \&\& cd build
# cmake .. -G Ninja -DCMAKE\_BUILD\_TYPE=Release -DRNG\_WITH\_OPENCL=OFF
# cmake --build . --parallel
# ./rng\_selftest.exe
# If AVX2 is supported, it will automatically be compiled in and used.
# 
# Usage Example
# cpp
# \#include "universal\_rng.h"
# \#include <iostream>
# 
# int main() {
# &nbsp;   // Create RNG instance (seed, algorithm\_id, bitwidth)
# &nbsp;   universal\_rng\_t\* rng = universal\_rng\_new(1337, 0, 1);
# 
# &nbsp;   // Generate 64-bit random integer
# &nbsp;   uint64\_t val = universal\_rng\_next\_u64(rng);
# &nbsp;   std::cout << "Random u64: " << val << std::endl;
# 
# &nbsp;   // Generate double in range \[0,1)
# &nbsp;   double d = universal\_rng\_next\_double(rng);
# &nbsp;   std::cout << "Random double: " << d << std::endl;
# 
# &nbsp;   // Cleanup
# &nbsp;   universal\_rng\_free(rng);
# }
# Replace C++ Standard RNG
# To use universal\_rng\_lib in place of std::mt19937 or std::default\_random\_engine:
# 
# Replace all instances of:
# 
# cpp
# std::mt19937 rng(seed);
# with:
# 
# cpp
# auto\* rng = universal\_rng\_new(seed, 0, 1);  // use algorithm 0 = Xoroshiro128++
# Replace:
# 
# cpp
# rng(); // or dist(rng)
# with:
# 
# cpp
# universal\_rng\_next\_u64(rng);
# Use universal\_rng\_next\_double(rng); for floating-point needs.
# 
# Replace delete rng; with:
# 
# cpp
# universal\_rng\_free(rng);
# File Structure
# text
#
# X.
# ├── include/                # All public headers
# │   └── universal\_rng.h    # Main header
# ├── Benchmarking/           # Benchmarking Results 
# │                           [compared against C++ std. lib]
# ├── src/                    # Source code
# │   ├── simd\_avx2.cpp
# │   ├── simd\_sse2.cpp
# │   ├── simd\_avx512.cpp
# │   ├── universal\_rng.cpp
# │   └── runtime\_detect.cpp
# ├── lib\_files/              # Prebuilt binaries
# │   └── mingw\_shared/
# │   └── msvc\_shared/
# │   └── linux\_shared/
# ├── extras/                 # Environment setups and tools
# │   └── windows/
# ├── docs/                   # In-depth design documentation
# │   ├── key\_SIMD-implementation\_design-principles.md
# │   ├── explain\_of\_3-7's\_refactor.md
# │   └── opencl-implementation-details.md
# └── tests/                  # Self-test and benchmarks
#
# SIMD \& Dispatch Design
# Auto-detects best available instruction set at runtime
# 
# Gracefully falls back to scalar or SSE2
# 
# Batches can be used to further accelerate performance
# 
# Detection failures are handled gracefully
# 
# Example detection result:
# 
# yaml
# CPU feature detection:
# &nbsp; SSE2: Yes
# &nbsp; AVX2: Yes
# &nbsp; AVX512: No
# Trying AVX2 implementation...
# Using AVX2 implementation
# Benchmarking \& Performance
# Batch mode yields 1.7×–2.5× speedup over naive generation
# 
# AVX2 performs ~3–5× faster than std::mt19937
# 
# AVX-512 versions under development
# 
# License
# MIT License – see LICENSE.md for full terms.
# 
# Reference
# This library is partially inspired by:
# 
# David Blackman \& Sebastiano Vigna's paper on Scrambled Linear PRNGs (SLRNG)
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
