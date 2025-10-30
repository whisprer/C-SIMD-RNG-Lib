#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <chrono>
#if defined(_MSC_VER)
  #include <intrin.h>
#else
  #include <x86intrin.h>
#endif
#include "ua/ua_rng.h"
#include "ua/ua_platform.h"

#ifndef UA_BENCH_N
#define UA_BENCH_N 8000000
#endif
#ifndef UA_REPS
#define UA_REPS 7
#endif
#ifndef UA_WARM
#define UA_WARM 2
#endif

struct Run { double ms; double cpe; };

template<class F>
Run time_once(F&& f, std::size_t elems) {
  auto t0 = std::chrono::high_resolution_clock::now();
  unsigned long long c0 = __rdtsc();
  f();
  unsigned long long c1 = __rdtsc();
  auto t1 = std::chrono::high_resolution_clock::now();
  double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
  double cycles = double(c1 - c0);
  return { ms, cycles / double(elems) };
}

static double sum_double(const double* p, std::size_t n) {
  double s = 0.0;
  for (std::size_t i=0;i<n;++i) s += p[i];
  return s;
}
static std::uint64_t sum_u64(const std::uint64_t* p, std::size_t n) {
  std::uint64_t s = 0;
  for (std::size_t i=0;i<n;++i) s ^= p[i];
  return s;
}

static void summarize(const char* label, double* ms, double* cpe, int reps, std::size_t n, ua::SimdTier tier) {
  std::vector<int> idx(reps); for (int i=0;i<reps;++i) idx[i]=i;
  auto cmp = [&](int a, int b){ return ms[a] < ms[b]; };
  std::sort(idx.begin(), idx.end(), cmp);
  int best = idx[0];
  double med_ms = ms[idx[reps/2]];
  double med_cpe = cpe[idx[reps/2]];
  const char* simd = (tier==ua::SimdTier::AVX512F?"AVX512F":(tier==ua::SimdTier::AVX2?"AVX2":"Scalar"));
  std::printf("%-12s | n=%zu | simd=%s | best: %.3f ms (%.2f cyc/elem) | median: %.3f ms (%.2f)\n",
              label, n, simd, ms[best], cpe[best], med_ms, med_cpe);
}

int main() {
  const std::size_t N = UA_BENCH_N;

  {
    ua::Rng rng(0x0123456789ABCDEFULL);
    std::vector<std::uint64_t> buf(N);
    for (int i=0;i<UA_WARM;++i) rng.generate_u64(buf.data(), N);
    double ms[UA_REPS], cpe[UA_REPS];
    for (int i=0;i<UA_REPS;++i) {
      auto r = time_once([&]{ rng.generate_u64(buf.data(), N); }, N);
      ms[i]=r.ms; cpe[i]=r.cpe;
    }
    summarize("u64", ms, cpe, UA_REPS, N, rng.simd_tier());
    std::printf("  checksum: 0x%016llx\n", (unsigned long long)sum_u64(buf.data(), N));
  }

  {
    ua::Rng rng(0xF00DBA5ECAFEBEEFULL);
    std::vector<double> buf(N);
    for (int i=0;i<UA_WARM;++i) rng.generate_double(buf.data(), N);
    double ms[UA_REPS], cpe[UA_REPS];
    for (int i=0;i<UA_REPS;++i) {
      auto r = time_once([&]{ rng.generate_double(buf.data(), N); }, N);
      ms[i]=r.ms; cpe[i]=r.cpe;
    }
    summarize("double[0,1)", ms, cpe, UA_REPS, N, rng.simd_tier());
    std::printf("  sum: %.6f\n", sum_double(buf.data(), N));
  }

  {
    ua::Rng rng(0xDEADBEEFDEADC0DEULL);
    std::vector<double> buf(N);
    for (int i=0;i<UA_WARM;++i) rng.generate_normal(buf.data(), N);
    double ms[UA_REPS], cpe[UA_REPS];
    for (int i=0;i<UA_REPS;++i) {
      auto r = time_once([&]{ rng.generate_normal(buf.data(), N); }, N);
      ms[i]=r.ms; cpe[i]=r.cpe;
    }
    summarize("normal N(0,1)", ms, cpe, UA_REPS, N, rng.simd_tier());
    std::printf("  sum: %.6f\n", sum_double(buf.data(), N));
  }

  return 0;
}
