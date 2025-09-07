force backend for testing:





UA\_FORCE\_BACKEND=scalar ./ua\_rng\_bench

UA\_FORCE\_BACKEND=avx2   ./ua\_rng\_bench

UA\_FORCE\_BACKEND=avx512 ./ua\_rng\_bench





\# Linux/macOS

cmake -S . -B build -DUA\_BUILD\_BENCH=ON -DUA\_ENABLE\_AVX2=ON -DUA\_ENABLE\_AVX512=ON

cmake --build build -j

./build/ua\_rng\_bench





Design



Backends live in separate translation units compiled with ISA flags.



The façade (ua::Rng) picks the best-supported backend at runtime via CPUID and OSXSAVE checks.



Doubles use exponent injection (53-bit mantissa) for speed \& reproducibility.



Normal uses Marsaglia Polar (scalar + AVX2 vectorized rejection).



Notes



If you ship binaries, keep UA\_ENABLE\_AVX2/AVX512 ON so the library contains those paths, but your client code stays portable.



Set UA\_STREAM\_STORES=1 to use streaming stores when buffers are 32/64-byte aligned.





\# what this gives you right now



\- \*\*Portability:\*\* One `ua\_rng` static lib works everywhere. It only uses AVX2/AVX-512 where the CPU+OS say it’s safe.

\- \*\*Speed:\*\* On AVX2/AVX-512 boxes you’ll see 4–8× lane throughput. Scalar path is solid and uses xoshiro256\*\* (great quality \& speed).

\- \*\*Normals:\*\* Vectorized polar on AVX2 (practical \& fast), scalar fallback everywhere else.

\- \*\*Clean API:\*\* `generate\_u64`, `generate\_double`, `generate\_normal`, `jump`, `simd\_tier()`.



\# how to drop into your repo



\- Replace your current `include/ua/\*`, `src/\*`, `bench/\*`, and top-level `CMakeLists.txt` with the ones above under `/v1.7/…`.

\- Keep your `docs/g-petey\_convo.md` alongside — the build doesn’t touch it.



\# next perf pushes we can do (once you’re ready)



\- add \*\*per-backend jump()\*\* for AVX paths (vectorized jump constants)

\- add \*\*Philox4x32-10\*\* AVX2 backend for counter-based parallel streams

\- add \*\*Ziggurat\*\* normals (tables in `.rodata`, AVX2 sampler)

\- implement \*\*aligned stream stores\*\* with runtime alignment detection + non-temporal write combining thresholds

\- consider \*\*batch API\*\* returning lambdas that fill N elements per call to reduce virtual indirection in hot loops



if you want me to fold this into a zip for you or tailor the bench to your exact `UA\_BENCH\_N/UA\_PIN\_CORE` ritual, say the word. also happy to wire in `add\_subdirectory(tests)` with chi-square / KS checks for nightly CI.







