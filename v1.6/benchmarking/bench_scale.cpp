// bench/bench_scale.cpp
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <memory>

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

static inline void set_force_tier(const char* v) {
#if defined(_WIN32)
  _putenv_s("UA_FORCE_SIMD", v);
#else
  setenv("UA_FORCE_SIMD", v, 1);
#endif
}

static inline const char* tier_name(ua::SimdTier t) {
  switch (t) {
    case ua::SimdTier::AVX512: return "AVX512";
    case ua::SimdTier::AVX2:   return "AVX2";
    case ua::SimdTier::NEON:   return "NEON";
    default:                   return "Scalar";
  }
}

// Very light CSV writer
struct Csv {
  FILE* f{};
  explicit Csv(const char* path) {
#if defined(_WIN32)
    fopen_s(&f, path, "w");
#else
    f = fopen(path, "w");
#endif
    if (f) {
      fprintf(f, "engine,algo,tier,mode,metric,threads,batch,count,ms,rate_per_s\n");
      fflush(f);
    }
  }
  ~Csv() { if (f) fclose(f); }
  void row(const char* engine, const char* algo, const char* tier,
           const char* mode, const char* metric, int threads, std::size_t batch,
           std::size_t count, double ms, double rate) {
    if (!f) return;
    fprintf(f, "%s,%s,%s,%s,%s,%d,%zu,%zu,%.6f,%.6f\n",
            engine, algo, tier, mode, metric, threads, batch, count, ms, rate);
  }
};

// Thread affinity on Windows (best-effort). On other OSes it’s a no-op.
static void pin_thread_to_core(int core_index) {
#if defined(_WIN32)
  DWORD_PTR mask = 1ull << (core_index % (sizeof(DWORD_PTR)*8));
  SetThreadAffinityMask(GetCurrentThread(), mask);
#else
  (void)core_index;
#endif
}

struct JobCfg {
  std::string tier_env;      // "scalar", "avx2", "avx512", "neon"
  ua::Algorithm algo;
  const char*   algo_name;
  std::size_t   n_per_thread;
  std::size_t   batch;
  int           threads;
};

// worker runs a single metric for N items
static void worker_u64(std::atomic<std::uint64_t>& sum, const JobCfg& cfg, int tid) {
  pin_thread_to_core(tid);
  ua::Init init{};
  init.algo = cfg.algo;
  init.seed = 0xC0FFEE1234567890ULL + (std::uint64_t)tid * 1315423911u;
  init.stream = 7 + (unsigned)tid;
  init.buffer.capacity_u64 = (std::max<std::size_t>)(cfg.batch, 1024);

  ua::UniversalRng rng(init);
  std::vector<std::uint64_t> tmp(cfg.batch);
  std::size_t done = 0;
  std::uint64_t local = 0;

  while (done < cfg.n_per_thread) {
    std::size_t take = (std::min)(cfg.batch, cfg.n_per_thread - done);
    rng.generate_u64(tmp.data(), take);
    for (std::size_t i=0;i<take;++i) local += tmp[i];
    done += take;
  }
  sum.fetch_add(local, std::memory_order_relaxed);
}

static void worker_double(std::atomic<double>& sum, const JobCfg& cfg, int tid) {
  pin_thread_to_core(tid);
  ua::Init init{};
  init.algo = cfg.algo;
  init.seed = 0xD15EA5E123456789ULL + (std::uint64_t)tid * 2654435761u;
  init.stream = 11 + (unsigned)tid;
  init.buffer.capacity_u64 = (std::max<std::size_t>)(cfg.batch, 1024);

  ua::UniversalRng rng(init);
  std::vector<double> tmp(cfg.batch);
  std::size_t done = 0;
  double local = 0.0;

  while (done < cfg.n_per_thread) {
    std::size_t take = (std::min)(cfg.batch, cfg.n_per_thread - done);
    rng.generate_double(tmp.data(), take);
    for (std::size_t i=0;i<take;++i) local += tmp[i];
    done += take;
  }
  sum.fetch_add(local, std::memory_order_relaxed);
}

static void worker_normal(std::atomic<double>& sum, std::atomic<double>& sum2, const JobCfg& cfg, int tid) {
  pin_thread_to_core(tid);
  ua::Init init{};
  init.algo = cfg.algo;
  init.seed = 0xBADC0DE123456789ULL + (std::uint64_t)tid * 11400714819323198485ull;
  init.stream = 19 + (unsigned)tid;
  init.buffer.capacity_u64 = (std::max<std::size_t>)(cfg.batch, 1024);

  ua::UniversalRng rng(init);
  std::vector<double> tmp(cfg.batch);
  std::size_t done = 0;
  double lsum = 0.0, lsum2 = 0.0;

  while (done < cfg.n_per_thread) {
    std::size_t take = (std::min)(cfg.batch, cfg.n_per_thread - done);
    rng.generate_normal(0.0, 1.0, tmp.data(), take);
    for (std::size_t i=0;i<take;++i) { double x = tmp[i]; lsum += x; lsum2 += x*x; }
    done += take;
  }
  sum.fetch_add(lsum, std::memory_order_relaxed);
  sum2.fetch_add(lsum2, std::memory_order_relaxed);
}

int main(int, char**) {
  const char* threads_env = std::getenv("UA_SCALE_THREADS");
  const char* batches_env = std::getenv("UA_SCALE_BATCHES");
  const char* npt_env     = std::getenv("UA_SCALE_N_PER_THREAD");
  const char* out_env     = std::getenv("UA_SCALE_OUT");

  std::vector<int> threads = {1,2,4,8};
  std::vector<std::size_t> batches = {256,1024,4096,16384,65536};
  std::size_t n_per_thread = npt_env ? std::strtoull(npt_env, nullptr, 10) : (1ull<<22); // 4M per thread
  std::string out = out_env ? out_env : "bench_scale_results.csv";

  auto parse_int_list = [](const char* s, auto& outv) {
    if (!s) return;
    outv.clear();
    const char* p = s;
    while (*p) {
      char* end = nullptr;
      long v = std::strtol(p, &end, 10);
      if (end==p) break;
      outv.push_back((typename std::decay<decltype(outv[0])>::type)v);
      if (*end == ',') p = end+1; else break;
    }
  };
  parse_int_list(threads_env, threads);
  parse_int_list(batches_env, batches);

  Csv csv(out.c_str());
  std::printf("Scaling bench -> %s\n", out.c_str());

  struct UAJob { ua::Algorithm algo; const char* name; };
  const UAJob jobs[] = {
    { ua::Algorithm::Xoshiro256ss,  "xoshiro256**" },
    { ua::Algorithm::Philox4x32_10, "philox4x32-10" }
  };

  std::vector<std::string> tiers = {"scalar"};
#if defined(UA_ENABLE_AVX2)
  tiers.push_back("avx2");
#endif
#if defined(UA_ENABLE_AVX512)
  tiers.push_back("avx512");
#endif
#if defined(__aarch64__) || defined(__ARM_NEON)
  tiers.push_back("neon");
#endif

  for (const auto& tier : tiers) {
    set_force_tier(tier.c_str());
    for (const auto& job : jobs) {
      for (int T : threads) {
        for (std::size_t B : batches) {
          // u64
          {
            std::atomic<std::uint64_t> sum{0};
            auto t0 = ms_now();
            std::vector<std::thread> pool;
            JobCfg cfg{tier, job.algo, job.name, n_per_thread, B, T};
            pool.reserve(T);
            for (int t=0; t<T; ++t) pool.emplace_back(worker_u64, std::ref(sum), std::cref(cfg), t);
            for (auto& th : pool) th.join();
            auto t1 = ms_now();
            const std::size_t total = (std::size_t)T * n_per_thread;
            double ms = (t1 - t0);
            double rate = total / (ms / 1000.0);
            std::printf("[UA] %-12s %-6s T=%d B=%6zu  u64   : %8.2f ms  (%.2f M/s)\n",
                        job.name, tier.c_str(), T, B, ms, rate/1e6);
            csv.row("UA", job.name, tier.c_str(), "batch", "u64", T, B, total, ms, rate);
          }
          // double
          {
            std::atomic<double> sum{0.0};
            auto t0 = ms_now();
            std::vector<std::thread> pool;
            JobCfg cfg{tier, job.algo, job.name, n_per_thread, B, T};
            pool.reserve(T);
            for (int t=0; t<T; ++t) pool.emplace_back(worker_double, std::ref(sum), std::cref(cfg), t);
            for (auto& th : pool) th.join();
            auto t1 = ms_now();
            const std::size_t total = (std::size_t)T * n_per_thread;
            double ms = (t1 - t0);
            double rate = total / (ms / 1000.0);
            std::printf("[UA] %-12s %-6s T=%d B=%6zu  dbl   : %8.2f ms  (%.2f M/s)\n",
                        job.name, tier.c_str(), T, B, ms, rate/1e6);
            csv.row("UA", job.name, tier.c_str(), "batch", "double", T, B, total, ms, rate);
          }
          // normal
          {
            std::atomic<double> sum{0.0}, sum2{0.0};
            auto t0 = ms_now();
            std::vector<std::thread> pool;
            JobCfg cfg{tier, job.algo, job.name, n_per_thread, B, T};
            pool.reserve(T);
            for (int t=0; t<T; ++t) pool.emplace_back(worker_normal, std::ref(sum), std::ref(sum2), std::cref(cfg), t);
            for (auto& th : pool) th.join();
            auto t1 = ms_now();
            const std::size_t total = (std::size_t)T * n_per_thread;
            double ms = (t1 - t0);
            double rate = total / (ms / 1000.0);
            double mean = sum.load() / double(total);
            double var  = sum2.load() / double(total) - mean*mean;
            std::printf("[UA] %-12s %-6s T=%d B=%6zu  norm  : %8.2f ms  (%.2f M/s)  mean=% .4f var=%.4f\n",
                        job.name, tier.c_str(), T, B, ms, rate/1e6, mean, var);
            csv.row("UA", job.name, tier.c_str(), "batch", "normal", T, B, total, ms, rate);
          }
        }
      }
    }
  }

  std::printf("\nCSV written: %s\n", out.c_str());
  return 0;
}
