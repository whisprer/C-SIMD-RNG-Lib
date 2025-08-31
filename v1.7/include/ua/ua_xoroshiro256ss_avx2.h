// v1.6.1/include/ua/ua_xoshiro256ss_avx2.h
#pragma once
#include <cstdint>
#include <cstddef>
#include <immintrin.h>
#include "ua_platform.h"

namespace ua {

struct Xoshiro256ssAVX2 {
  __m256i s0, s1, s2, s3;

  static UA_FORCE_INLINE __m256i rotl64(__m256i x, int k) noexcept {
    __m256i l = _mm256_slli_epi64(x, k);
    __m256i r = _mm256_srli_epi64(x, 64 - k);
    return _mm256_or_si256(l, r);
  }
  static UA_FORCE_INLINE __m256i mul_by_5(__m256i x) noexcept {
    __m256i x4 = _mm256_slli_epi64(x, 2);
    return _mm256_add_epi64(x4, x);
  }
  static UA_FORCE_INLINE __m256i mul_by_9(__m256i x) noexcept {
    __m256i x8 = _mm256_slli_epi64(x, 3);
    return _mm256_add_epi64(x8, x);
  }

  static uint64_t splitmix64_scalar(uint64_t& x) {
    x += 0x9e3779b97f4a7c15ULL;
    uint64_t z = x;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
  }

  explicit Xoshiro256ssAVX2(uint64_t seed) {
    uint64_t z = seed;
    uint64_t a0 = splitmix64_scalar(z);
    uint64_t a1 = splitmix64_scalar(z);
    uint64_t a2 = splitmix64_scalar(z);
    uint64_t a3 = splitmix64_scalar(z);
    uint64_t b0 = splitmix64_scalar(z);
    uint64_t b1 = splitmix64_scalar(z);
    uint64_t b2 = splitmix64_scalar(z);
    uint64_t b3 = splitmix64_scalar(z);

    s0 = _mm256_set_epi64x(a3, a2, a1, a0);
    s1 = _mm256_set_epi64x(b3, b2, b1, b0);
    s2 = _mm256_set_epi64x(splitmix64_scalar(z), splitmix64_scalar(z), splitmix64_scalar(z), splitmix64_scalar(z));
    s3 = _mm256_set_epi64x(splitmix64_scalar(z), splitmix64_scalar(z), splitmix64_scalar(z), splitmix64_scalar(z));
  }

  UA_FORCE_INLINE __m256i next_vec_u64() noexcept {
    __m256i res = mul_by_9(rotl64(mul_by_5(s1), 7));

    __m256i t = _mm256_slli_epi64(s1, 17);

    s2 = _mm256_xor_si256(s2, s0);
    s3 = _mm256_xor_si256(s3, s1);
    s1 = _mm256_xor_si256(s1, s2);
    s0 = _mm256_xor_si256(s0, s3);
    s2 = _mm256_xor_si256(s2, t);
    s3 = rotl64(s3, 45);

    return res;
  }

  void generate_u64(uint64_t* out, size_t n) noexcept {
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
      __m256i v = next_vec_u64();
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(out + i), v);
    }
    if (i < n) {
      __m256i v = next_vec_u64();
      alignas(32) uint64_t tmp[4];
      _mm256_store_si256(reinterpret_cast<__m256i*>(tmp), v);
      for (; i < n; ++i) out[i] = tmp[i & 3];
    }
  }

  void generate_double(double* out, size_t n) noexcept {
    constexpr double inv = 1.0 / static_cast<double>(1ULL << 53);
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
      __m256i v = next_vec_u64();
      alignas(32) uint64_t tmp[4];
      _mm256_store_si256(reinterpret_cast<__m256i*>(tmp), v);
      out[i+0] = static_cast<double>(tmp[0] >> 11) * inv;
      out[i+1] = static_cast<double>(tmp[1] >> 11) * inv;
      out[i+2] = static_cast<double>(tmp[2] >> 11) * inv;
      out[i+3] = static_cast<double>(tmp[3] >> 11) * inv;
    }
    for (; i < n; ++i) {
      alignas(32) uint64_t tmp[4];
      __m256i v = next_vec_u64();
      _mm256_store_si256(reinterpret_cast<__m256i*>(tmp), v);
      out[i] = static_cast<double>(tmp[i & 3] >> 11) * inv;
    }
  }
};

} // namespace ua
