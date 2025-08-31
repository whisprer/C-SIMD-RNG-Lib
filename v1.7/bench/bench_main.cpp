#include <cstdio>
#include <cstdint>
#include <chrono>
#include <mm_malloc.h>
#include <x86intrin.h>   // __rdtsc

#include "ua/ua_rng.h"

// -------- knobs ----------
#ifndef UA_BENCH_N
#define UA_BENCH_N 2000000
#endif
#ifndef UA_REPS
#define UA_REPS 7         // measured repetitions (best-of and median reported)
#endif
#ifndef UA_WARM
#define UA_WARM 2         // warmup reps
#endif
// Define UA_PIN_CORE=<0..63> to pin the thread on Windows
// Define UA_STREAM_STORES=1 at compile-time to enable streaming stores in AVX2 path

using hr_clock = std::chrono::high_resolution_clock;

static void* aligned_malloc_bytes(size_t bytes, size_t align=64){ return _mm_malloc(bytes,(int)align); }
static void  aligned_free(void* p){ _mm_free(p); }

static uint64_t checksum_u64(const uint64_t* v, size_t n){
  uint64_t c=0xcbf29ce484222325ull;
  for (size_t i=0;i<n;++i){ c ^= v[i]; c *= 0x100000001b3ull; }
  return c;
}
static double sum_double(const double* v, size_t n){
  long double s=0.0L; for(size_t i=0;i<n;++i) s+=v[i]; return double(s);
}

#ifdef _WIN32
  #include <windows.h>
  static void raise_prio_and_pin(){
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    #ifdef UA_PIN_CORE
      DWORD_PTR mask = (DWORD_PTR)1ull << (UA_PIN_CORE & 63);
      SetThreadAffinityMask(GetCurrentThread(), mask);
    #endif
  }
#else
  static void raise_prio_and_pin(){}
#endif

struct RunStats {
  double ms;
  double cycles_per_elem;
};

template<class F>
static RunStats time_once(F&& fn, size_t elems){
  auto t0 = hr_clock::now();
  uint64_t c0 = __rdtsc();
  fn();
  uint64_t c1 = __rdtsc();
  auto t1 = hr_clock::now();
  double ms = std::chrono::duration<double,std::milli>(t1-t0).count();
  double cpe = double(c1 - c0) / double(elems);
  return {ms, cpe};
}

static void summarize(const char* label, const double* ms, const double* cpe, int n, size_t elems, int tier){
  // compute best (min), median
  auto ms_copy = std::vector<double>(ms, ms+n);
  auto cpe_copy = std::vector<double>(cpe, cpe+n);
  std::nth_element(ms_copy.begin(), ms_copy.begin()+n/2, ms_copy.end());
  std::nth_element(cpe_copy.begin(), cpe_copy.begin()+n/2, cpe_copy.end());
  double ms_best = *std::min_element(ms, ms+n);
  double ms_med  = ms_copy[n/2];
  double cpe_best = *std::min_element(cpe, cpe+n);
  double cpe_med  = cpe_copy[n/2];
  double rate_best = double(elems) / (ms_best*1e-3);
  double rate_med  = double(elems) / (ms_med*1e-3);
  std::printf("%s: N=%zu  best %.3f ms (%.2f M/s)  med %.3f ms (%.2f M/s)  [tier=%d]\n",
              label, elems, ms_best, rate_best/1e6, ms_med, rate_med/1e6, tier);
  std::printf("      cycles/elem: best %.2f  med %.2f\n", cpe_best, cpe_med);
}

int main(){
  std::printf("cfg: N=%d  AVX2=%d  STREAM=%d  SCALAR=%d",
    int(UA_BENCH_N),
  #ifdef __AVX2__
    1,
  #else
    0,
  #endif
  #if defined(UA_STREAM_STORES) && UA_STREAM_STORES
    1,
  #else
    0,
  #endif
  #ifdef UA_FORCE_SCALAR
    1
  #else
    0
  #endif
  );
  #ifdef UA_PIN_CORE
    std::printf("  PIN_CORE=%d", UA_PIN_CORE);
  #endif
  std::puts("");

  raise_prio_and_pin();

  const size_t N = UA_BENCH_N;
  // warmup
  { ua::Rng warm(1); uint64_t tmp[1024]; warm.generate_u64(tmp, 1024); }

  // ---- u64
  {
    ua::Rng rng(0x123456789abcdef0ULL);
    uint64_t* buf = (uint64_t*)aligned_malloc_bytes(N*sizeof(uint64_t), 64);
    // warm runs
    for (int i=0;i<UA_WARM;++i) rng.generate_u64(buf, N);
    double ms[UA_REPS], cpe[UA_REPS];
    for (int i=0;i<UA_REPS;++i){
      auto r = time_once([&]{ rng.generate_u64(buf, N); }, N);
      ms[i]=r.ms; cpe[i]=r.cycles_per_elem;
    }
    summarize("u64", ms, cpe, UA_REPS, N, rng.simd_tier());
    std::printf("  checksum: %016llx\n", (unsigned long long)checksum_u64(buf, N));
    aligned_free(buf);
  }
  // ---- double
  {
    ua::Rng rng(0x123456789abcdef0ULL);
    double* buf = (double*)aligned_malloc_bytes(N*sizeof(double), 64);
    for (int i=0;i<UA_WARM;++i) rng.generate_double(buf, N);
    double ms[UA_REPS], cpe[UA_REPS];
    for (int i=0;i<UA_REPS;++i){
      auto r = time_once([&]{ rng.generate_double(buf, N); }, N);
      ms[i]=r.ms; cpe[i]=r.cycles_per_elem;
    }
    summarize("double", ms, cpe, UA_REPS, N, rng.simd_tier());
    std::printf("  sum: %.6f\n", sum_double(buf, N));
    aligned_free(buf);
  }
  return 0;
}
