#pragma once
#include <cstdint>
#include <cstddef>
#include <bit>
#include <immintrin.h>

namespace ua {

// splitmix64 for seeding
static inline uint64_t ua_splitmix64(uint64_t& x) noexcept {
  x += 0x9e3779b97f4a7c15ull;
  uint64_t z = x;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
  return z ^ (z >> 31);
}

// ===================== xoroshiro128++ scalar =====================
struct Xoroshiro128ppScalar {
  uint64_t s0, s1;

  static inline uint64_t rotl(uint64_t v, int k) noexcept {
    return (v << k) | (v >> (64 - k));
  }

  explicit Xoroshiro128ppScalar(uint64_t seed) noexcept {
    uint64_t z = seed;
    s0 = ua_splitmix64(z);
    s1 = ua_splitmix64(z);
  }

  inline uint64_t next_u64() noexcept {
    uint64_t res = rotl(s0 + s1, 17) + s0;
    s1 ^= s0;
    s0 = rotl(s0, 49) ^ s1 ^ (s1 << 21);
    s1 = rotl(s1, 28);
    return res;
  }

  void generate_u64(uint64_t* out, size_t n) noexcept {
    for (size_t i = 0; i < n; ++i) out[i] = next_u64();
  }

  // [0,1) doubles via exponent injection (52-bit mantissa)
  void generate_double(double* out, size_t n) noexcept {
    for (size_t i = 0; i < n; ++i) {
      uint64_t x = next_u64();
      uint64_t bits = (x >> 12) | 0x3FF0000000000000ULL; // [1,2)
      out[i] = std::bit_cast<double>(bits) - 1.0;        // -> [0,1)
    }
  }
};

#if defined(__AVX2__)
// ===================== xoroshiro128++ AVX2 (unrolled) =====================
struct Xoroshiro128ppAVX2 {
  __m256i s0, s1;

  static inline __m256i rotl64(__m256i v, int k) noexcept {
    __m256i l = _mm256_slli_epi64(v, k);
    __m256i r = _mm256_srli_epi64(v, 64 - k);
    return _mm256_or_si256(l, r);
  }

  explicit Xoroshiro128ppAVX2(uint64_t seed) noexcept {
    uint64_t z = seed;
    uint64_t a0 = ua_splitmix64(z), a1 = ua_splitmix64(z), a2 = ua_splitmix64(z), a3 = ua_splitmix64(z);
    uint64_t b0 = ua_splitmix64(z), b1 = ua_splitmix64(z), b2 = ua_splitmix64(z), b3 = ua_splitmix64(z);
    s0 = _mm256_set_epi64x(a3, a2, a1, a0);
    s1 = _mm256_set_epi64x(b3, b2, b1, b0);
  }

  inline __m256i next_vec_u64() noexcept {
    __m256i sum = _mm256_add_epi64(s0, s1);
    __m256i res = _mm256_add_epi64(rotl64(sum, 17), s0);
    s1 = _mm256_xor_si256(s1, s0);
    __m256i s0r = rotl64(s0, 49);
    __m256i s1l = _mm256_slli_epi64(s1, 21);
    s0 = _mm256_xor_si256(_mm256_xor_si256(s0r, s1), s1l);
    s1 = rotl64(s1, 28);
    return res;
  }

  // ----- store helpers (u64 and double) -----
  static inline void store256_u64(uint64_t* p, __m256i v) noexcept {
  #if defined(UA_STREAM_STORES) && UA_STREAM_STORES
    if ((((uintptr_t)p) & 31u) == 0u) { _mm256_stream_si256((__m256i*)p, v); return; }
  #endif
    _mm256_storeu_si256((__m256i*)p, v);
  }

  static inline void store256_pd(double* p, __m256d v) noexcept {
  #if defined(UA_STREAM_STORES) && UA_STREAM_STORES
    if ((((uintptr_t)p) & 31u) == 0u) { _mm256_stream_pd(p, v); return; }
  #endif
    _mm256_storeu_pd(p, v);
  }

  // ----- u64 generation (unrolled 8x: 32 u64 per loop) -----
  void generate_u64(uint64_t* out, size_t n) noexcept {
    size_t i = 0;
    while (i + 32 <= n) {
      __m256i v0 = next_vec_u64(); __m256i v1 = next_vec_u64();
      __m256i v2 = next_vec_u64(); __m256i v3 = next_vec_u64();
      __m256i v4 = next_vec_u64(); __m256i v5 = next_vec_u64();
      __m256i v6 = next_vec_u64(); __m256i v7 = next_vec_u64();
      store256_u64(out + i +  0, v0); store256_u64(out + i +  4, v1);
      store256_u64(out + i +  8, v2); store256_u64(out + i + 12, v3);
      store256_u64(out + i + 16_
