// v1.6.1/include/ua/ua_xoshiro256ss_scalar.h
#pragma once
#include <cstdint>
#include <cstddef>
#include "ua_platform.h"

namespace ua {

struct Xoshiro256ssScalar {
  uint64_t s0, s1, s2, s3;

  static UA_FORCE_INLINE uint64_t rotl(uint64_t x, int k) noexcept {
    return (x << k) | (x >> (64 - k));
  }

  static uint64_t splitmix64(uint64_t& x) {
    x += 0x9e3779b97f4a7c15ULL;
    uint64_t z = x;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
  }

  explicit Xoshiro256ssScalar(uint64_t seed) {
    uint64_t z = seed;
    s0 = splitmix64(z);
    s1 = splitmix64(z);
    s2 = splitmix64(z);
    s3 = splitmix64(z);
  }

  UA_FORCE_INLINE uint64_t next_u64() noexcept {
    uint64_t result = rotl(s1 * 5ULL, 7) * 9ULL;
    uint64_t t = s1 << 17;
    s2 ^= s0;
    s3 ^= s1;
    s1 ^= s2;
    s0 ^= s3;
    s2 ^= t;
    s3 = rotl(s3, 45);
    return result;
  }

  void generate_u64(uint64_t* out, size_t n) noexcept {
    for (size_t i=0;i<n;++i) out[i] = next_u64();
  }

  void generate_double(double* out, size_t n) noexcept {
    constexpr double inv = 1.0 / static_cast<double>(1ULL << 53);
    for (size_t i=0;i<n;++i) {
      uint64_t x = next_u64();
      out[i] = static_cast<double>(x >> 11) * inv; // [0,1)
    }
  }
};

} // namespace ua
