#include <chrono>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include "ua/ua_rng.h"

using clock_type = std::chrono::high_resolution_clock;

static void bench_u64(const char* label, ua::Rng& rng, size_t count) {
  std::vector<uint64_t> buf(count);
  auto t0 = clock_type::now();
  rng.generate_u64(buf.data(), buf.size());
  auto t1 = clock_type::now();
  double ms = std::chrono::duration<double,std::milli>(t1 - t0).count();
  double rate = double(count) / (ms/1000.0);
  std::printf("%s: %zu u64 in %.3f ms  => %.2f M/s  [tier=%d]\n", label, count, ms, rate/1e6, rng.simd_tier());
  uint64_t acc = 0; for (auto x: buf) acc ^= x + 0x9e3779b97f4a7c15ULL + (acc<<6) + (acc>>2);
  std::printf("  checksum: %016llx\n", (unsigned long long)acc);
}

static void bench_double(const char* label, ua::Rng& rng, size_t count) {
  std::vector<double> buf(count);
  auto t0 = clock_type::now();
  rng.generate_double(buf.data(), buf.size());
  auto t1 = clock_type::now();
  double ms = std::chrono::duration<double,std::milli>(t1 - t0).count();
  double rate = double(count) / (ms/1000.0);
  std::printf("%s: %zu doubles in %.3f ms  => %.2f M/s  [tier=%d]\n", label, count, ms, rate/1e6, rng.simd_tier());
  volatile double sink = 0; for (auto x: buf) sink += x;
  std::printf("  sum: %.6f\n", sink);
}

extern "C" {
  int ua_cuda_philox_generate_u64(uint64_t seed, uint64_t* host_out, size_t n);
  int ua_opencl_philox_generate_u64(uint64_t seed, uint64_t* host_out, size_t n);
}

int main() {
  const size_t N = 2'000'000;

  // CPU
  ua::Rng cpu(0x123456789abcdef0ULL);
  bench_u64("CPU batch u64", cpu, N);
  bench_double("CPU batch double", cpu, N);

#if defined(UA_HAS_CUDA)
  std::vector<uint64_t> gpu(N);
  if (ua_cuda_philox_generate_u64(0xABCDEF0011223344ULL, gpu.data(), N)==0) {
    auto t0 = clock_type::now();
    volatile uint64_t acc=0; for (auto v: gpu) acc ^= v;
    auto t1 = clock_type::now();
    double ms = std::chrono::duration<double,std::milli>(t1 - t0).count();
    std::printf("CUDA philox: %zu u64 copied in %.3f ms  => host-xor ok\n", N, ms);
  } else {
    std::puts("CUDA not available or failed.");
  }
#endif

#if defined(UA_HAS_OPENCL)
  std::vector<uint64_t> clbuf(N);
  if (ua_opencl_philox_generate_u64(0x5555AAAACCCC3333ULL, clbuf.data(), N)==0) {
    auto t0 = clock_type::now();
    volatile uint64_t acc=0; for (auto v: clbuf) acc ^= v;
    auto t1 = clock_type::now();
    double ms = std::chrono::duration<double,std::milli>(t1 - t0).count();
    std::printf("OpenCL philox: %zu u64 copied in %.3f ms  => host-xor ok\n", N, ms);
  } else {
    std::puts("OpenCL not available or failed.");
  }
#endif

  return 0;
}
