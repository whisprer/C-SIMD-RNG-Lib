/docs/CONTRIBUTING.md
# Contributing to UA RNG Library

Thank you for your interest in contributing to UA RNG!  
This document provides guidelines for contributing code, reporting issues, and collaborating on this high-performance random number generation project.

---

## Table of Contents

- [Code of Conduct](#code-of-conduct)  
- [Getting Started](#getting-started)  
- [Development Setup](#development-setup)  
- [Contributing Code](#contributing-code)  
- [Performance Standards](#performance-standards)  
- [Testing Requirements](#testing-requirements)  
- [Documentation](#documentation)  
- [Issue Reporting](#issue-reporting)  
- [Development Workflow](#development-workflow)  
- [Recognition](#recognition)  

---

## Code of Conduct

This project values:

- **Technical excellence** and performance-focused development  
- **Cross-platform portability** with runtime SIMD dispatch  
- **Clear communication** about optimization tradeoffs  
- **Constructive collaboration** on algorithms and benchmarks  

---

## Getting Started

### Prerequisites

- **C++20** compatible compiler  
  - GCC 10+, Clang 12+, MSVC 2019+  
- **CMake 3.16+**  
- **Git** for version control  

### Development Environment Setup

```bash
# Clone the repository
git clone https://github.com/yourname/ua_rng.git
cd ua_rng

# Create development build
mkdir build-dev && cd build-dev
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# Run tests
ctest

Development Setup
Platform-Specific Notes
Windows (MSVC / MinGW)

Use /arch:AVX2 (MSVC) or -mavx2 -march=native (MinGW) for best performance.

AVX-512 requires /arch:AVX512 (MSVC) or -mavx512f (MinGW).

Linux (GCC / Clang)

Recommended flags: -O3 -march=native -funroll-loops

Always build with position-independent code (-fPIC) for shared libraries.

macOS (Clang / AppleClang)

AVX2 supported on Intel Macs; AVX-512 only on recent Intel CPUs.

Apple Silicon defaults to scalar path (no NEON support yet in v1.7).

Contributing Code
Branch Strategy

main → stable, tested releases

develop → integration branch for next release

feature/xxx → new features or algorithms

perf/xxx → performance optimizations

hotfix/xxx → urgent fixes

Coding Standards
C++ Guidelines

C++20 features encouraged (concepts, ranges, constexpr, etc.)

RAII for all resource management

Smart pointers where ownership matters

Consistent naming: snake_case for functions, PascalCase for classes

SIMD Implementation
// Template for new SIMD backend
class BackendAvx2 {
public:
    explicit BackendAvx2(uint64_t seed);
    void generate_u64(uint64_t* dest, size_t count);
    std::string name() const;
private:
    // SIMD state, aligned memory, helpers
};

Performance Expectations

Alignment: 64-byte for AVX-512, 32-byte for AVX2

Branch elimination: minimize conditions in hot paths

Cache efficiency: prefer sequential access

SIMD utilization: target theoretical speedup ratios

Commit Message Format
type(scope): brief description

Details:
- Performance impact
- Platform compatibility notes
- Breaking changes (if any)

Performance: [metrics]
Platforms: [tested platforms]


Types: feat, fix, perf, docs, style, refactor, test, build

Example:

perf(avx2): vectorize normal distribution polar sampler

Improved batch throughput by ~2.4x on AVX2 CPUs.
Platforms: Linux GCC, Windows MSVC

Performance Standards
Benchmark Requirements

Before/after benchmarks included in PR description.

Cross-platform testing (Linux, Windows, macOS where possible).

No regressions in scalar path.

Performance Targets

Single u64: match xoshiro256** reference

AVX2 batch: ~4× scalar

AVX-512 batch: ~8× scalar

Normals: vectorized Polar faster than scalar fallback

Testing Requirements

New contributions must pass:

Correctness tests – matches reference outputs

Statistical tests – uniformity, independence

Performance tests – meets throughput targets

Cross-platform builds – Linux, Windows, macOS

Memory safety – no leaks, proper cleanup

Documentation

Header comments → algorithm references, citations

Inline comments → non-trivial SIMD intrinsics

Usage examples in README and docs

Issue Reporting

When filing an issue, include:

Platform + compiler details

Minimal reproduction case

Expected vs actual behavior

Performance metrics (if relevant)

Labels:

bug, perf, build, api, simd, platform

Development Workflow
Pull Requests

Fork + create feature branch

Implement with tests + benchmarks

Run full test suite (ctest)

Update docs if needed

Submit PR with results

Address review feedback

Merge after approval

Review Criteria

Correctness

Performance impact

Portability

Code quality

Documentation

Recognition

Contributors are acknowledged in:

Project README.md

Release notes

Performance “hall of fame” for significant speedups


---