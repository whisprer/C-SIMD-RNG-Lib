# Universal Architecture PRNG – High-Performance C++ RNG Library
A high-performance, cross-platform random number generation library with advanced SIMD and optional GPU acceleration — designed as a drop-in replacement for the C++ Standard Library RNGs in high-throughput workloads.

Overview
universal_rng_lib is a modern C++ random number generation library engineered for speed, scalability, and architectural adaptability. It implements multiple high-quality PRNG algorithms, automatically selecting the fastest available CPU instruction set at runtime, with optional GPU acceleration for extreme workloads.

The library is particularly optimized for batch random number generation, where it can vastly outperform std::mt19937 and other C++ standard RNGs — especially in simulations, procedural content generation, large-scale statistical analysis, and scientific computing.

Two major versions are currently available:

v1.5 – Stable, widely tested, consistently outperforms C++ standard RNGs for SIMD workloads.

v1.6 – Experimental, introducing a modular architecture for broader algorithm coverage, new SIMD optimizations, and multi-thread scaling. Not yet universally faster than v1.5 in all cases.

Features
Multiple PRNG Algorithms

Xoroshiro128++

WyRand

Philox4x32-10

Advanced SIMD Acceleration

SSE2, AVX2, AVX-512, NEON (auto-detected at runtime)

GPU Offload (Optional)

OpenCL back-end for massive parallelism

Portable Scalar Fallback

Works on any architecture, including non-SIMD CPUs

Batch Generation Support

Significantly improved throughput when generating large arrays of random values

Multiple Output Formats

16–1024 bit generation

Cross-Platform

Windows (MSVC, MinGW) and Linux

MIT Licensed

Version Comparison
Feature / Metric	v1.5 (Stable)	v1.6 (Experimental)
SIMD Support	SSE2, AVX2, AVX-512, NEON	SSE2, AVX2, AVX-512, NEON
GPU OpenCL Support	✅	✅
Algorithms	Xoroshiro128++, WyRand	Xoroshiro128++, WyRand, Philox4x32-10
Multi-thread Scaling	Limited	✅ Affinity-based scaling
SIMD Batch Throughput	Excellent	Excellent to Exceptional
Single Number Generation	Competitive, but std::mt19937 can win	Competitive, but std::mt19937 can win
Portability	Mature, well-tested	Incomplete — more OS/arch work needed
Recommended For	Production workloads	Performance testing, R&D

Performance Notes
Best Use Cases:

Procedural content generation in games

Large-scale Monte Carlo simulations

Financial modeling and statistical analysis

Any SIMD-friendly bulk number generation workload

When to Prefer std::mt19937 or Other std RNGs:

If your workload primarily generates single random numbers at infrequent intervals on non-SIMD CPUs.

SIMD vs. Scalar:

Batch SIMD performance can be orders of magnitude faster than scalar generation.

As workloads shrink to single number generation, C++’s std::mt19937 can match or surpass these PRNGs.

Building the Library
bash
Copy
Edit
# Clone the repository
git clone https://github.com/yourname/universal_rng_lib.git
cd universal_rng_lib

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release
This produces both static (.lib / .a) and dynamic (.dll / .so) library builds.

Linking and Usage
Example (MSVC, static link):

cpp
Copy
Edit
#include "universal_rng.hpp"

int main() {
    universal_rng rng;
    uint64_t val = rng.next_u64();
    double dbl = rng.next_double();
}
Compile and link:

powershell
Copy
Edit
cl /O2 /I include main.cpp build/Release/universal_rng.lib
License
MIT License – See LICENSE file for details.

