/docs/Development\_Roadmap.md

\# UA RNG Library – Development Roadmap



This roadmap outlines the current state of UA RNG (v1.7) and the priorities for future releases.  

It is intended as a living document to guide contributors and users on where the library is headed.



---



\## Current Status (v1.7)



\*\*Working Features:\*\*

\- Modern C++20 façade: `ua::Rng` with scalar, AVX2, and AVX-512F backends

\- Runtime feature detection via CPUID + OSXSAVE

\- Fast batch generation: `generate\_u64`, `generate\_double`, `generate\_normal`

\- Normals via Marsaglia Polar method (vectorized on AVX2)

\- Jump-ahead subsequences (scalar baseline)

\- Clean CMake build system for Linux, macOS, Windows



\*\*Current Limitations:\*\*

\- SIMD `jump()` not yet implemented

\- No GPU backend (OpenCL/CUDA removed in 1.7)

\- No cryptographic PRNGs (ChaCha, AES, etc.)

\- Streaming stores still experimental

\- Limited algorithm diversity (xoshiro/xoroshiro family only)



---



\## Immediate Priorities



\### 1. SIMD Feature Parity

\- Add per-backend `jump()` constants for AVX2/AVX-512

\- Ensure reproducibility between scalar and SIMD streams



\### 2. Algorithm Expansion

\- \*\*Philox4x32-10 (AVX2)\*\* for parallel counter-based streams

\- \*\*Ziggurat normals\*\* (table-driven, AVX2 vectorized)  

\- \*\*PCG family\*\* as an additional high-quality baseline



\### 3. Performance Tuning

\- Optimize streaming stores with runtime alignment detection

\- Explore cache-aware write combining for batch fills

\- Reduce virtual indirection in hot loops (batch API refinements)



---



\## Medium-Term Development



\### 4. API \& Architecture

\- Extended batch API (lambda fillers, flexible buffer sizes)

\- Error handling and diagnostic hooks

\- Debug/profiling build variants



\### 5. Cryptographic PRNGs

\- ChaCha20 PRNG

\- AES-based PRNG (leveraging AES-NI)

\- RDRAND/RDSEED passthrough wrappers



\### 6. Extended Platforms

\- \*\*Rust bindings\*\* (via cxx or FFI)

\- \*\*Python bindings\*\* (pybind11)

\- \*\*WebAssembly SIMD\*\* for browser deployment

\- \*\*ARM NEON\*\* backend for Apple Silicon / ARM64 servers



---



\## Long-Term Vision



\### 7. Multi-Language Ecosystem

\- Rust native implementation with `std::simd`

\- WASM module for JS/TS usage

\- Go \& C# bindings for ecosystem adoption



\### 8. Advanced Optimizations

\- NUMA-aware memory allocation

\- Lock-free parallel generation

\- Thread-local RNG instances with reproducible seeds

\- Profile-guided optimization (PGO) builds



\### 9. Quality \& Reliability

\- Statistical test suite (TestU01, PractRand)

\- Continuous integration across Linux/macOS/Windows

\- Cross-compiler verification (GCC, Clang, MSVC)

\- Performance regression benchmarks in CI



---



\## Performance Targets



\*\*Batch Generation\*\*

\- AVX2: 4× speedup over scalar

\- AVX-512F: 8× speedup over scalar

\- Normals (Polar): ≥2× faster on AVX2 vs scalar



\*\*Single Value\*\*

\- Match or exceed xoshiro256\*\* reference performance



\*\*Cryptographic PRNGs\*\*

\- Comparable to ChaCha20 reference implementations

\- AES PRNG: leverage hardware AES-NI for minimal overhead



---



\## Success Metrics



\- Deterministic reproducibility across backends

\- Zero memory leaks or UB across platforms

\- Clear performance documentation for all algorithms

\- Consistent user API regardless of backend



---



\*This roadmap evolves with each release. Contributions, suggestions, and experiments are always welcome.\*

