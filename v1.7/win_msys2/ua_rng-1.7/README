# UA RNG v1.7

A fast, portable C++20 random number library with **runtime SIMD dispatch**:

- Scalar baseline (xoshiro256**)
- AVX2 4× lanes
- AVX-512F 8× lanes (optional)
- Streams: `u64`, `[0,1)` `double`, and `N(0,1)` normals (polar)
- `jump()` for 2^128 subsequence

## Usage

```cpp
#include "ua/ua_rng.h"
ua::Rng rng(1234);
std::vector<uint64_t> v(1<<20);
rng.generate_u64(v.data(), v.size());
Force a backend at runtime (for testing):

ini
Copy code
UA_FORCE_BACKEND=scalar ./ua_rng_bench
UA_FORCE_BACKEND=avx2   ./ua_rng_bench
UA_FORCE_BACKEND=avx512 ./ua_rng_bench
Build
bash
Copy code
# Linux/macOS
cmake -S . -B build -DUA_BUILD_BENCH=ON -DUA_ENABLE_AVX2=ON -DUA_ENABLE_AVX512=ON
cmake --build build -j
./build/ua_rng_bench
powershell
Copy code
# Windows Developer Powershell (MSVC)
cmake -S . -B build
cmake --build build --config Release
.\build\Release\ua_rng_bench.exe
Design
Backends live in separate translation units compiled with ISA flags.

The façade (ua::Rng) picks the best-supported backend at runtime via CPUID and OSXSAVE checks.

Doubles use exponent injection (53-bit mantissa) for speed & reproducibility.

Normal uses Marsaglia Polar (scalar + AVX2 vectorized rejection).

Notes
If you ship binaries, keep UA_ENABLE_AVX2/AVX512 ON so the library contains those paths, but your client code stays portable.

Set UA_STREAM_STORES=1 to use streaming stores when buffers are 32/64-byte aligned.

markdown
Copy code

---

# what this gives you right now

- **Portability:** One `ua_rng` static lib works everywhere. It only uses AVX2/AVX-512 where the CPU+OS say it’s safe.
- **Speed:** On AVX2/AVX-512 boxes you’ll see 4–8× lane throughput. Scalar path is solid and uses xoshiro256** (great quality & speed).
- **Normals:** Vectorized polar on AVX2 (practical & fast), scalar fallback everywhere else.
- **Clean API:** `generate_u64`, `generate_double`, `generate_normal`, `jump`, `simd_tier()`.

# how to drop into your repo

- Replace your current `include/ua/*`, `src/*`, `bench/*`, and top-level `CMakeLists.txt` with the ones above under `/v1.7/…`.
- Keep your `docs/g-petey_convo.md` alongside — the build doesn’t touch it.

# next perf pushes we can do (once you’re ready)

- add **per-backend jump()** for AVX paths (vectorized jump constants)
- add **Philox4x32-10** AVX2 backend for counter-based parallel streams
- add **Ziggurat** normals (tables in `.rodata`, AVX2 sampler)
- implement **aligned stream stores** with runtime alignment detection + non-temporal write combining thresholds
- consider **batch API** returning lambdas that fill N elements per call to reduce virtual indirection in hot loops



