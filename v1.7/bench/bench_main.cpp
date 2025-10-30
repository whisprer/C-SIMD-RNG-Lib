#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <cmath>
#include <chrono>
#include <cstdlib>

#include <x86intrin.h>     // __rdtsc

#include "ua/ua_rng.h"

// ------------------------------------------------------------
// simple helpers
// ------------------------------------------------------------
template<class F>
static inline std::pair<double,double> time_once(F&& fn, std::size_t elems) {
    // returns {milliseconds, cycles_per_element}
    unsigned long long c0 = __rdtsc();
    auto t0 = std::chrono::high_resolution_clock::now();

    fn();

    auto t1 = std::chrono::high_resolution_clock::now();
    unsigned long long c1 = __rdtsc();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double cycles = double(c1 - c0);
    double cpe = cycles / double(elems);
    return {ms, cpe};
}

static std::uint64_t sum_u64(const std::uint64_t* p, std::size_t n) {
    std::uint64_t s = 0;
    for (std::size_t i=0;i<n;i++) s ^= p[i] + 0x9e3779b97f4a7c15ull + (s<<6) + (s>>2);
    return s;
}

#include <cmath> // ensure present

static double sum_f64(const double* p, std::size_t n) {
    long double s = 0.0L;
    for (std::size_t i = 0; i < n; ++i) {
        double v = p[i];
        if (std::isfinite(v)) s += (long double)v;  // ignore NaN/Inf defensively
    }
    return (double)s;
}

static const char* tier_name(ua::SimdTier t) {
    switch (t) {
        case ua::SimdTier::Scalar:  return "Scalar";
        case ua::SimdTier::AVX2:    return "AVX2";
        case ua::SimdTier::AVX512F: return "AVX512";
        default: return "?";
    }
}

static void summarize(const char* label, double* ms, double* cpe, int reps, std::size_t n, ua::SimdTier tier) {
    std::vector<double> vms(ms, ms+reps);
    std::vector<double> vc(cpe, cpe+reps);
    std::nth_element(vms.begin(), vms.begin() + reps/2, vms.end());
    std::nth_element(vc.begin(),  vc.begin()  + reps/2, vc.end());
    double best_ms = *std::min_element(ms, ms+reps);
    double med_ms  = vms[reps/2];
    double best_c  = *std::min_element(cpe, cpe+reps);
    double med_c   = vc[reps/2];
    std::printf("%-12s | n=%zu | simd=%s | best: %.3f ms (%.2f cyc/elem) | median: %.3f ms (%.2f)\n",
                label, n, tier_name(tier), best_ms, best_c, med_ms, med_c);
}

// ------------------------------------------------------------
// optional: quick normality stats (enable with UA_BENCH_STATS=1)
// ------------------------------------------------------------
static void stats_normal(ua::Rng& rng, std::size_t n) {
    std::vector<double> x(n);
    rng.normal(x.data(), n);

    long double s1 = 0.0L, s2 = 0.0L, s3 = 0.0L, s4 = 0.0L;
    for (double v : x) {
        long double t = v;
        s1 += t;
        s2 += t * t;
        s3 += t * t * t;
        s4 += t * t * t * t;
    }

    long double mean  = s1 / n;
    long double var   = s2 / n - mean * mean;
    long double stdev = std::sqrt((double)var);
    long double skew  = (s3 / n) / (stdev * stdev * stdev);
    long double kurt  = (s4 / n) / (var * var) - 3.0L;

    std::printf("normal stats: n=%zu mean=%.6g std=%.6g skew=%.6g kurt=%.6g\n",
                n, (double)mean, (double)stdev, (double)skew, (double)kurt);
}

// ------------------------------------------------------------
// bench
// ------------------------------------------------------------
int main() {
    constexpr std::size_t N     = 8'000'000;
    constexpr int UA_WARM = 2;
    constexpr int UA_REPS = 12;

    ua::Rng rng(123456789);

    // ---------- u64 ----------
    {
        std::vector<std::uint64_t> buf(N);
        for (int i=0;i<UA_WARM;++i) rng.generate_u64(buf.data(), N);
        double ms[UA_REPS], cpe[UA_REPS];
        for (int r=0;r<UA_REPS;++r) {
            auto rpair = time_once([&]{ rng.generate_u64(buf.data(), N); }, N);
            ms[r]  = rpair.first;
            cpe[r] = rpair.second;
        }
        summarize("u64", ms, cpe, UA_REPS, N, rng.simd_tier());
        std::printf("  checksum: 0x%016llx\n", (unsigned long long)sum_u64(buf.data(), N));
    }

    // ---------- double[0,1) ----------
    {
        std::vector<double> buf(N);
        for (int i=0;i<UA_WARM;++i) rng.generate_double(buf.data(), N);
        double ms[UA_REPS], cpe[UA_REPS];
        for (int r=0;r<UA_REPS;++r) {
            auto rpair = time_once([&]{ rng.generate_double(buf.data(), N); }, N);
            ms[r]  = rpair.first;
            cpe[r] = rpair.second;
        }
        summarize("double[0,1)", ms, cpe, UA_REPS, N, rng.simd_tier());
        std::printf("  sum: %.6f\n", sum_f64(buf.data(), N));
    }

    // ---------- normal N(0,1) ----------
    {
        std::vector<double> buf(N);
        for (int i=0;i<UA_WARM;++i) rng.generate_normal(buf.data(), N);
        double ms[UA_REPS], cpe[UA_REPS];
        for (int r=0;r<UA_REPS;++r) {
            auto rpair = time_once([&]{ rng.generate_normal(buf.data(), N); }, N);
            ms[r]  = rpair.first;
            cpe[r] = rpair.second;
        }
        summarize("normal N(0,1)", ms, cpe, UA_REPS, N, rng.simd_tier());
        std::printf("  sum: %.6f\n", sum_f64(buf.data(), N));
    }

    if (std::getenv("UA_BENCH_STATS")) {
        stats_normal(rng, 5'000'000);
    }

    return 0;
}
