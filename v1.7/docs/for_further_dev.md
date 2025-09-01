/docs/for_further_dev.md
# UA RNG – Further Development Ideas

This document collects speculative ideas, experiments, and possible long-term directions for UA RNG beyond the main roadmap.  
It is less a set of commitments and more a space for forward-looking exploration.

---

## 1. SIMD Extensions & Specialization

- **Per-backend `jump()`**: vectorized jump constants for AVX2/AVX-512 streams.
- **Wider SIMD batching**: explore AVX-512VL/DQ subsets for flexible register use.
- **ARM NEON & SVE**: bring the scalar + SIMD design to ARM platforms.
- **RISC-V V**: anticipate future vector extensions for reproducible RNG streams.

---

## 2. Alternative Algorithms

- **Philox4x32-10** (counter-based, parallel-friendly).  
- **Ziggurat normals** (fast Gaussian sampling with precomputed tables).  
- **PCG family** (well-studied small-state generators).  
- **SplitMix variants** for seeding and lightweight tasks.  

Cryptographic candidates:
- **ChaCha20** PRNG for secure simulation use cases.  
- **AES-CTR PRNG** using AES-NI hardware instructions.  
- **RDRAND/RDSEED passthrough** for hardware entropy.

---

## 3. Memory & Storage Optimizations

- **Streaming stores** with adaptive thresholds based on buffer size/alignment.  
- **NUMA-aware allocation** for high-core count servers.  
- **Lock-free ring buffers** for parallel number generation.  
- **Cache-conscious data layouts** to reduce eviction under large batch workloads.

---

## 4. Multi-Language & Ecosystem Integration

- **Rust bindings** (via cxx or native port with `std::simd`).  
- **Python bindings** (via pybind11).  
- **WebAssembly SIMD** for in-browser scientific computing.  
- **C# / Go interop** for HPC or service back-ends.

---

## 5. Advanced Use Cases

- **Monte Carlo at scale**: reproducibility across heterogeneous nodes.  
- **Procedural content generation**: reproducible seeds with SIMD acceleration.  
- **Statistical simulation frameworks**: plug-in RNG backends.  
- **Benchmark harness**: runtime feature detection + comparative analysis.  

---

## 6. Experimental Optimizations

- **Profile-guided optimization (PGO)**: feed benchmark traces back into builds.  
- **Thread-local PRNGs**: avoid contention in multi-threaded workloads.  
- **Batch API with lambdas**: user-supplied functors fill buffers in-place.  
- **Deterministic parallel streams**: partitioned seeding for reproducibility.

---

## 7. Testing & Validation

- Integrate **PractRand** and **TestU01** in CI.  
- Add **cross-platform reproducibility checks** (scalar vs AVX2 vs AVX-512).  
- Record **performance baselines per CPU family** to catch regressions.  

---

## Notes

- These ideas are not guaranteed for the next release but guide where UA RNG could grow.  
- Community contributions in any of these areas are welcome, whether experimental PRs, benchmarks, or design notes.

---