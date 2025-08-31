#pragma once
#include <cstdint>
#include <cstddef>
#include "ua_xoroshiro128pp.h"

namespace ua {

class Rng {
public:
#if defined(UA_FORCE_SCALAR)
  using Backend = Xoroshiro128ppScalar; static constexpr int kTier = 1;
#elif defined(__AVX2__)
  using Backend = Xoroshiro128ppAVX2;   static constexpr int kTier = 2;
#else
  using Backend = Xoroshiro128ppScalar; static constexpr int kTier = 1;
#endif

  explicit Rng(uint64_t seed) : impl_(seed) {}
  int simd_tier() const noexcept { return kTier; }
  void generate_u64(uint64_t* out, size_t n) noexcept { impl_.generate_u64(out, n); }
  void generate_double(double* out, size_t n) noexcept { impl_.generate_double(out, n); }

private:
  Backend impl_;
};

} // namespace ua
