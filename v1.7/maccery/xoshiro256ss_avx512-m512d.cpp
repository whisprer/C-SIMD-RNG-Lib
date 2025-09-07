#include "ua/ua_xoshiro256ss_avx512.h"
#include <cmath>
#include <cstring>


static inline __m512d approx_log_pd(__m512d x) noexcept {
  __m512d one = _mm512_set1_pd(1.0);
  __m512d y = _mm512_sub_pd(x, one);
  __m512d y2 = _mm512_mul_pd(y, y);
  __m512d y3 = _mm512_mul_pd(y2, y);
  __m512d y4 = _mm512_mul_pd(y2, y2);
  __m512d y5 = _mm512_mul_pd(y4, y);
  return _mm512_add_pd(y,
         _mm512_add_pd(_mm512_mul_pd(_mm512_set1_pd(-0.5), y2),
         _mm512_add_pd(_mm512_mul_pd(_mm512_set1_pd(0.3333333333), y3),
         _mm512_add_pd(_mm512_mul_pd(_mm512_set1_pd(-0.25), y4),
                       _mm512_mul_pd(_mm512_set1_pd(0.2), y5)))));
}

static inline __m512d approx_sqrt_pd(__m512d x) noexcept {
  __m256 rsqrt_ps = _mm256_rsqrt_ps(_mm512_cvtpd_ps(x)); // downcast to ps
  __m512d rsqrt = _mm512_cvtps_pd(rsqrt_ps);
  __m512d half = _mm512_set1_pd(0.5);
  __m512d threehalves = _mm512_set1_pd(1.5);
  __m512d r2 = _mm512_mul_pd(rsqrt, rsqrt);
  rsqrt = _mm512_mul_pd(rsqrt,
          _mm512_sub_pd(threehalves, _mm512_mul_pd(half, _mm512_mul_pd(x, r2))));
  return _mm512_mul_pd(x, rsqrt);
}

namespace ua::detail {

static inline __m512i rotl64(__m512i x, int k) noexcept {
  return _mm512_ternarylogic_epi64(
    _mm512_slli_epi64(x, k),
    _mm512_srli_epi64(x, 64 - k),
    _mm512_setzero_si512(),
    0xF8
  );
}
static inline __m512i mullo64(__m512i a, std::uint64_t c) noexcept {
  return _mm512_mullo_epi64(a, _mm512_set1_epi64((long long)c));
}

Xoshiro256ssAVX512::Xoshiro256ssAVX512(std::uint64_t seed) noexcept {
  auto sm64 = [](std::uint64_t& v) {
    v += 0x9e3779b97f4a7c15ull;
    std::uint64_t z = v;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
  };
  std::uint64_t x = seed;
  alignas(64) std::uint64_t tmp[32];
  for (int lane = 0; lane < 8; ++lane) {
    tmp[lane + 0]  = sm64(x);
    tmp[lane + 8]  = sm64(x);
    tmp[lane + 16] = sm64(x);
    tmp[lane + 24] = sm64(x);
  }
  s0 = _mm512_set_epi64(tmp[24],tmp[16],tmp[8],tmp[0],  tmp[25],tmp[17],tmp[9],tmp[1]);
  s1 = _mm512_set_epi64(tmp[26],tmp[18],tmp[10],tmp[2], tmp[27],tmp[19],tmp[11],tmp[3]);
  s2 = _mm512_set_epi64(tmp[28],tmp[20],tmp[12],tmp[4], tmp[29],tmp[21],tmp[13],tmp[5]);
  s3 = _mm512_set_epi64(tmp[30],tmp[22],tmp[14],tmp[6], tmp[31],tmp[23],tmp[15],tmp[7]);
}

__m512i Xoshiro256ssAVX512::next_u64_vec() noexcept {
  __m512i t1 = mullo64(s1, 5u);
  __m512i res = mullo64(rotl64(t1, 7), 9u);
  __m512i t = _mm512_slli_epi64(s1, 17);
  s2 = _mm512_xor_si512(s2, s0);
  s3 = _mm512_xor_si512(s3, s1);
  s1 = _mm512_xor_si512(s1, s2);
  s0 = _mm512_xor_si512(s0, s3);
  s2 = _mm512_xor_si512(s2, t);
  s3 = rotl64(s3, 45);
  return res;
}

void Xoshiro256ssAVX512::generate_u64(std::uint64_t* out, std::size_t n) noexcept {
  std::size_t i = 0;
  while (i + 8 <= n) {
    __m512i v = next_u64_vec();
    _mm512_storeu_si512(reinterpret_cast<void*>(out + i), v);
    i += 8;
  }
  if (i < n) {
    alignas(64) std::uint64_t tmp[8];
    _mm512_store_si512(reinterpret_cast<void*>(tmp), next_u64_vec());
    for (; i < n; ++i) out[i] = tmp[i & 7];
  }
}

void Xoshiro256ssAVX512::generate_double(double* out, std::size_t n) noexcept {
  constexpr std::uint64_t EXP = 0x3FFull << 52;
  std::size_t i = 0;
  while (i + 8 <= n) {
    __m512i u    = _mm512_srli_epi64(next_u64_vec(), 12);
    __m512i bits = _mm512_or_epi64(u, _mm512_set1_epi64((long long)EXP));
    __m512d d    = _mm512_sub_pd(_mm512_castsi512_pd(bits), _mm512_set1_pd(1.0));
    _mm512_storeu_pd(out + i, d);
    i += 8;
  }
  if (i < n) {
    alignas(64) double tmp[8];
    __m512i u    = _mm512_srli_epi64(next_u64_vec(), 12);
    __m512i bits = _mm512_or_epi64(u, _mm512_set1_epi64((long long)EXP));
    __m512d d    = _mm512_sub_pd(_mm512_castsi512_pd(bits), _mm512_set1_pd(1.0));
    _mm512_store_pd(tmp, d);
    for (; i < n; ++i) out[i] = tmp[i & 7];
  }
}

namespace ua::detail {

// Simple scalar Polar using this backend's uniform for portability.
void Xoshiro256ssAVX512::generate_normal(double* out, std::size_t n) noexcept {
  std::size_t i = 0;
  while (i < n) {
    double u, v, s;
    do {
      u = 2.0*uniform_scalar() - 1.0;
      v = 2.0*uniform_scalar() - 1.0;
      s = u*u + v*v;
    } while (s == 0.0 || s >= 1.0);
    double k = std::sqrt(-2.0 * std::log(s) / s);
    out[i++] = u*k;
    if (i < n) out[i++] = v*k;
  }
}

double Xoshiro256ssAVX512::uniform_scalar() noexcept {
  constexpr std::uint64_t EXP = 0x3FFull << 52;
  alignas(64) std::uint64_t tmp[8];
  _mm512_store_si512(reinterpret_cast<void*>(tmp), _mm512_srli_epi64(next_u64_vec(), 12));
  std::uint64_t bits = tmp[0] | EXP;
  double d; std::memcpy(&d, &bits, sizeof(d)); return d - 1.0;
}

} // namespace ua::detail
