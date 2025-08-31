#pragma once
#include <cstdint>
#include <cstddef>

#if defined(__AVX2__)
  #include <immintrin.h>
#endif

namespace ua {

// ---------- splitmix64 for seeding ----------
static inline uint64_t ua_splitmix64(uint64_t& x) noexcept {
  x += 0x9e3779b97f4a7c15ull;
  uint64_t z = x;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
  return z ^ (z >> 31);
}

// ---------- xoroshiro128++ scalar ----------
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
  void generate_double(double* out, size_t n) noexcept {
    constexpr double inv = 1.0 / double(1ull << 53);
    for (size_t i = 0; i < n; ++i) out[i] = double(next_u64() >> 11) * inv;
  }
};

#if defined(__AVX2__)
// ---------- xoroshiro128++ AVX2 (4 lanes) ----------
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
    __m256i res = _mm256_add_epi64(_mm256_or_si256(_mm256_slli_epi64(sum,17), _mm256_srli_epi64(sum,47)), s0);
    s1 = _mm256_xor_si256(s1, s0);
    __m256i s0r = _mm256_or_si256(_mm256_slli_epi64(s0,49), _mm256_srli_epi64(s0,15));
    __m256i s1l = _mm256_slli_epi64(s1, 21);
    s0 = _mm256_xor_si256(_mm256_xor_si256(s0r, s1), s1l);
    s1 = _mm256_or_si256(_mm256_slli_epi64(s1,28), _mm256_srli_epi64(s1,36));
    return res;
  }

  void generate_u64(uint64_t* out, size_t n) noexcept {
    size_t i = 0;
    for (; i + 4 <= n; i += 4) _mm256_storeu_si256((__m256i*)(out + i), next_vec_u64());
    if (i < n) {
      alignas(32) uint64_t t[4];
      _mm256_store_si256((__m256i*)t, next_vec_u64());
      for (; i < n; ++i) out[i] = t[i & 3];
    }
  }
  void generate_double(double* out, size_t n) noexcept {
    constexpr double inv = 1.0 / double(1ull << 53);
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
      alignas(32) uint64_t t[4];
      _mm256_store_si256((__m256i*)t, next_vec_u64());
      out[i+0] = double(t[0] >> 11) * inv;
      out[i+1] = double(t[1] >> 11) * inv;
      out[i+2] = double(t[2] >> 11) * inv;
      out[i+3] = double(t[3] >> 11) * inv;
    }
    if (i < n) {
      alignas(32) uint64_t t[4];
      _mm256_store_si256((__m256i*)t, next_vec_u64());
      out[i] = double(t[i & 3] >> 11) * inv;
    }
  }
};
#endif // __AVX2__

// ---------- Uniform wrapper API expected by your bench ----------
class Rng {
public:
#if defined(__AVX2__)
  using Backend = Xoroshiro128ppAVX2;
  static constexpr int kTier = 2;
#else
  using Backend = Xoroshiro128ppScalar;
  static constexpr int kTier = 1;
#endif

  explicit Rng(uint64_t seed) : impl_(seed) {}
  int simd_tier() const noexcept { return kTier; }
  void generate_u64(uint64_t* out, size_t n) noexcept { impl_.generate_u64(out, n); }
  void generate_double(double* out, size_t n) noexcept { impl_.generate_double(out, n); }

private:
  Backend impl_;
};

} // namespace ua
