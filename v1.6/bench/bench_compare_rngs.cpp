// bench/bench_compare_rngs.cpp
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <fstream>
#include <algorithm>

#include "universal_rng.h"
#include "runtime_detect.h"

#if defined(_WIN32)
  #ifndef NOMINMAX
  #define NOMINMAX 1
  #endif
  #include <windows.h>
#endif

using Clock = std::chrono::steady_clock;

static inline double ms_now() {
  using namespace std::chrono;
  return duration<double, std::milli>(Clock::now().time_since_epoch()).count();
}

// SplitMix64 fold to make stable 64-bit checksums quickly
static inline std::uint64_t splitmix64(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

static std::uint64_t hash_u64_span(const std::uint64_t* p, std::size_t n) {
  std::uint64_t h = 0x123456789ABCDEF0ULL;
  for (std::size_t i = 0; i < n; ++i) h = splitmix64(h ^ p[i]);
  return h;
}
static std::uint64_t hash_double_span(const double* p, std::size_t n) {
  std::uint64_t h = 0xCAFEBABEDEADBEEFULL, u;
  for (std::size_t i = 0; i < n; ++i) {
    std::memcpy(&u, &p[i], sizeof(u));
    h = splitmix64(h ^ u);
  }
  return h;
}

// simple CSV writer
struct Csv {
  std::ofstream f;
  explicit Csv(const char* path) : f(path, std::ios::out | std::ios::trunc) {
    f << "engine,algo,tier,mode,metric,count,ms,rate_per_s,checksum\n";
  }
  void row(const std::string& engine, const std::string& algo, const std::string& tier,
           const std::string& mode, const std::string& metric, std::size_t count,
           double ms, double rate, std::uint64_t chk) {
    f << engine << ',' << algo << ',' << tier << ',' << mode << ','
      << metric << ',' << count << ',' << ms << ',' << rate << ','
      << std::hex << chk << std::dec << '\n';
  }
};

// map tier enum to string
static const char* tier_name(ua::SimdTier t) {
  switch (t) {
    case ua::SimdTier::AVX512: return "AVX512";
    case ua::SimdTier::AVX2:   return "AVX2";
    case ua::SimdTier::NEON:   return "NEON";
    default:                   return "Scalar";
  }
}

// force UA tier via env (effective on next UniversalRng construction)
static void set_force_tier(const char* v) {
#if defined(_WIN32)
  _putenv_s("UA_FORCE_SIMD", v);
#else
  setenv("UA_FORCE_SIMD", v, 1);
#endif
}

// ----------- UA RNG bench ------------
struct UAJob {
  ua::Algorithm algo;
  const char*   algo_name;
};

static void bench_ua_one(Csv& csv, const UAJob& job,
                         const char* force_tier_env, std::size_t N, std::size_t batch)
{
  set_force_tier(force_tier_env);

  ua::Init init{};
  init.algo = job.algo;
  init.seed = 0xDEADBEEFCAFEBABEULL;
  init.stream = 7;
  init.buffer.capacity_u64 = 8192;

  ua::UniversalRng rng(init);
  auto tsel = rng.simd_tier();

  std::vector<std::uint64_t> u(N);
  std::vector<double>        d(N);
  std::vector<double>        nrm(N);

  // batch u64
  {
    double t0 = ms_now();
    for (std::size_t i = 0; i < N; ) {
      std::size_t take = (std::min)(batch, N - i);
      rng.generate_u64(u.data() + i, take);
      i += take;
    }
    double t1 = ms_now();
    auto chk = hash_u64_span(u.data(), u.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[UA] %-12s %-7s %-6s  batch u64   : %8.2f ms  (%.2f M/s)  chk=%016llx\n",
                job.algo_name, tier_name(tsel), "batch", (t1 - t0), rate / 1e6,
                (unsigned long long)chk);
    csv.row("UA", job.algo_name, tier_name(tsel), "batch", "u64", N, (t1 - t0), rate, chk);
  }

  // single u64
  {
    double t0 = ms_now();
    for (std::size_t i = 0; i < N; ++i) rng.generate_u64(u.data() + i, 1);
    double t1 = ms_now();
    auto chk = hash_u64_span(u.data(), u.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[UA] %-12s %-7s %-6s  single u64 : %8.2f ms  (%.2f M/s)  chk=%016llx\n",
                job.algo_name, tier_name(tsel), "single", (t1 - t0), rate / 1e6,
                (unsigned long long)chk);
    csv.row("UA", job.algo_name, tier_name(tsel), "single", "u64", N, (t1 - t0), rate, chk);
  }

  // batch dbl
  {
    double t0 = ms_now();
    for (std::size_t i = 0; i < N; ) {
      std::size_t take = (std::min)(batch, N - i);
      rng.generate_double(d.data() + i, take);
      i += take;
    }
    double t1 = ms_now();
    auto chk = hash_double_span(d.data(), d.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[UA] %-12s %-7s %-6s  batch dbl  : %8.2f ms  (%.2f M/s)  chk=%016llx\n",
                job.algo_name, tier_name(tsel), "batch", (t1 - t0), rate / 1e6,
                (unsigned long long)chk);
    csv.row("UA", job.algo_name, tier_name(tsel), "batch", "double", N, (t1 - t0), rate, chk);
  }

  // single dbl
  {
    double t0 = ms_now();
    for (std::size_t i = 0; i < N; ++i) rng.generate_double(d.data() + i, 1);
    double t1 = ms_now();
    auto chk = hash_double_span(d.data(), d.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[UA] %-12s %-7s %-6s  single dbl : %8.2f ms  (%.2f M/s)  chk=%016llx\n",
                job.algo_name, tier_name(tsel), "single", (t1 - t0), rate / 1e6,
                (unsigned long long)chk);
    csv.row("UA", job.algo_name, tier_name(tsel), "single", "double", N, (t1 - t0), rate, chk);
  }

  // batch norm
  {
    double t0 = ms_now();
    for (std::size_t i = 0; i < N; ) {
      std::size_t take = (std::min)(batch, N - i);
      rng.generate_normal(0.0, 1.0, nrm.data() + i, take);
      i += take;
    }
    double t1 = ms_now();
    long double sum=0, sum2=0;
    for (double x: nrm) { sum += x; sum2 += x*x; }
    double mean = double(sum / N);
    double var  = double(sum2 / N - mean*mean);
    auto chk = hash_double_span(nrm.data(), nrm.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[UA] %-12s %-7s %-6s  batch norm : %8.2f ms  (%.2f M/s)  chk=%016llx  mean=% .5f var=%.5f\n",
                job.algo_name, tier_name(tsel), "batch", (t1 - t0), rate / 1e6,
                (unsigned long long)chk, mean, var);
    csv.row("UA", job.algo_name, tier_name(tsel), "batch", "normal", N, (t1 - t0), rate, chk);
  }

  // single norm
  {
    double t0 = ms_now();
    for (std::size_t i = 0; i < N; ++i) rng.generate_normal(0.0, 1.0, nrm.data() + i, 1);
    double t1 = ms_now();
    long double sum=0, sum2=0;
    for (double x: nrm) { sum += x; sum2 += x*x; }
    double mean = double(sum / N);
    double var  = double(sum2 / N - mean*mean);
    auto chk = hash_double_span(nrm.data(), nrm.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[UA] %-12s %-7s %-6s  single norm: %8.2f ms  (%.2f M/s)  chk=%016llx  mean=% .5f var=%.5f\n",
                job.algo_name, tier_name(tsel), "single", (t1 - t0), rate / 1e6,
                (unsigned long long)chk, mean, var);
    csv.row("UA", job.algo_name, tier_name(tsel), "single", "normal", N, (t1 - t0), rate, chk);
  }
}

// ----------- std:: RNG bench ------------
template<class URBG>
static void bench_std_one(Csv& csv, const char* eng_name, std::uint64_t seed,
                          std::size_t N, std::size_t batch)
{
  URBG eng(seed);
  std::vector<std::uint64_t> u(N);
  std::vector<double> d(N), nrm(N);
  std::uniform_real_distribution<double> U01(0.0, 1.0);
  std::normal_distribution<double> N01(0.0, 1.0);

  // u64 via eng()
  {
    double t0 = ms_now();
    for (std::size_t i=0;i<N;) {
      std::size_t take = (std::min)(batch, N - i);
      for (std::size_t k=0;k<take;++k) u[i+k] = (std::uint64_t)eng();
      i += take;
    }
    double t1 = ms_now();
    auto chk = hash_u64_span(u.data(), u.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[STD] %-12s %-7s %-6s  batch u64   : %8.2f ms  (%.2f M/s)  chk=%016llx\n",
                eng_name, "-", "batch", (t1 - t0), rate / 1e6, (unsigned long long)chk);
    csv.row("STD", eng_name, "-", "batch", "u64", N, (t1 - t0), rate, chk);
  }
  {
    URBG eng2(seed);
    double t0 = ms_now();
    for (std::size_t i=0;i<N;++i) u[i] = (std::uint64_t)eng2();
    double t1 = ms_now();
    auto chk = hash_u64_span(u.data(), u.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[STD] %-12s %-7s %-6s  single u64 : %8.2f ms  (%.2f M/s)  chk=%016llx\n",
                eng_name, "-", "single", (t1 - t0), rate / 1e6, (unsigned long long)chk);
    csv.row("STD", eng_name, "-", "single", "u64", N, (t1 - t0), rate, chk);
  }

  // double U(0,1)
  {
    URBG eng3(seed);
    double t0 = ms_now();
    for (std::size_t i=0;i<N;) {
      std::size_t take = (std::min)(batch, N - i);
      for (std::size_t k=0;k<take;++k) d[i+k] = U01(eng3);
      i += take;
    }
    double t1 = ms_now();
    auto chk = hash_double_span(d.data(), d.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[STD] %-12s %-7s %-6s  batch dbl  : %8.2f ms  (%.2f M/s)  chk=%016llx\n",
                eng_name, "-", "batch", (t1 - t0), rate / 1e6, (unsigned long long)chk);
    csv.row("STD", eng_name, "-", "batch", "double", N, (t1 - t0), rate, chk);
  }
  {
    URBG eng4(seed);
    double t0 = ms_now();
    for (std::size_t i=0;i<N;++i) d[i] = U01(eng4);
    double t1 = ms_now();
    auto chk = hash_double_span(d.data(), d.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[STD] %-12s %-7s %-6s  single dbl : %8.2f ms  (%.2f M/s)  chk=%016llx\n",
                eng_name, "-", "single", (t1 - t0), rate / 1e6, (unsigned long long)chk);
    csv.row("STD", eng_name, "-", "single", "double", N, (t1 - t0), rate, chk);
  }

  // normal(0,1)
  {
    URBG eng5(seed);
    double t0 = ms_now();
    for (std::size_t i=0;i<N;) {
      std::size_t take = (std::min)(batch, N - i);
      for (std::size_t k=0;k<take;++k) nrm[i+k] = N01(eng5);
      i += take;
    }
    double t1 = ms_now();
    long double sum=0, sum2=0;
    for (double x: nrm) { sum += x; sum2 += x*x; }
    double mean = double(sum / N);
    double var  = double(sum2 / N - mean*mean);
    auto chk = hash_double_span(nrm.data(), nrm.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[STD] %-12s %-7s %-6s  batch norm : %8.2f ms  (%.2f M/s)  chk=%016llx  mean=% .5f var=%.5f\n",
                eng_name, "-", "batch", (t1 - t0), rate / 1e6, (unsigned long long)chk, mean, var);
    csv.row("STD", eng_name, "-", "batch", "normal", N, (t1 - t0), rate, chk);
  }
  {
    URBG eng6(seed);
    double t0 = ms_now();
    for (std::size_t i=0;i<N;++i) nrm[i] = N01(eng6);
    double t1 = ms_now();
    long double sum=0, sum2=0;
    for (double x: nrm) { sum += x; sum2 += x*x; }
    double mean = double(sum / N);
    double var  = double(sum2 / N - mean*mean);
    auto chk = hash_double_span(nrm.data(), nrm.size());
    double rate = (N) / ((t1 - t0) / 1000.0);
    std::printf("[STD] %-12s %-7s %-6s  single norm: %8.2f ms  (%.2f M/s)  chk=%016llx  mean=% .5f var=%.5f\n",
                eng_name, "-", "single", (t1 - t0), rate / 1e6, (unsigned long long)chk, mean, var);
    csv.row("STD", eng_name, "-", "single", "normal", N, (t1 - t0), rate, chk);
  }
}

int main(int, char**) {
  const std::size_t N   = std::getenv("UA_BENCH_N")   ? std::strtoull(std::getenv("UA_BENCH_N"),   nullptr, 10) : (1ull<<23); // 8M
  const std::size_t B   = std::getenv("UA_BENCH_B")   ? std::strtoull(std::getenv("UA_BENCH_B"),   nullptr, 10) : 8192;       // batch
  const std::uint64_t SEED = std::getenv("UA_BENCH_SEED") ? std::strtoull(std::getenv("UA_BENCH_SEED"), nullptr, 16) : 0xDEADBEEFCAFEBABEULL;

  std::printf("N=%zu  batch=%zu  seed=0x%016llx\n\n", N, B, (unsigned long long)SEED);

  Csv csv("bench_results.csv");

  const UAJob jobs[] = {
    { ua::Algorithm::Xoshiro256ss,  "xoshiro256**" },
    { ua::Algorithm::Philox4x32_10, "philox4x32-10" }
  };

  for (auto& j : jobs) bench_ua_one(csv, j, "scalar", N, B);
#if defined(UA_ENABLE_AVX2)
  for (auto& j : jobs) bench_ua_one(csv, j, "avx2", N, B);
#endif
#if defined(UA_ENABLE_AVX512)
  for (auto& j : jobs) bench_ua_one(csv, j, "avx512", N, B);
#endif
#if defined(__aarch64__) || defined(__ARM_NEON)
  for (auto& j : jobs) bench_ua_one(csv, j, "neon", N, B);
#endif

  bench_std_one<std::mt19937_64>(csv, "mt19937_64", SEED, N, B);
  bench_std_one<std::mt19937>   (csv, "mt19937",    (std::uint64_t)SEED, N, B);
  bench_std_one<std::ranlux48>  (csv, "ranlux48",   (std::uint64_t)SEED, N, B);
  bench_std_one<std::minstd_rand>(csv,"minstd_rand",(std::uint64_t)SEED, N, B);

  std::puts("\nCSV written: bench_results.csv");
  return 0;
}
