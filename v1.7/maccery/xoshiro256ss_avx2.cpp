// D:/code/Universal-Architecture-RNG-Lib/v1.7/src/xoshiro256ss_avx2.cpp
#include "ua/ua_xoshiro256ss_avx2.h"
#include <cmath>
#include <cstring>

namespace ua::detail {

// ----------------------------------------
// helpers
// ----------------------------------------
static inline __m256i rotl64(__m256i x, int k) noexcept {
  return _mm256_or_si256(_mm256_slli_epi64(x, k), _mm256_srli_epi64(x, 64 - k));
}
static inline __m256i mullo64(__m256i a, std::uint64_t c) noexcept {
  const __m256i lo_mask = _mm256_set1_epi64x(0xFFFFFFFFu);
  __m256i alo = _mm256_and_si256(a, lo_mask);
  __m256i ahi = _mm256_srli_epi64(a, 32);
  __m256i clo = _mm256_set1_epi64x(c & 0xFFFFFFFFu);
  __m256i chi = _mm256_set1_epi64x(c >> 32);
  __m256i lo = _mm256_mul_epu32(alo, clo);
  __m256i mid1 = _mm256_mul_epu32(alo, chi);
  __m256i mid2 = _mm256_mul_epu32(ahi, clo);
  __m256i mid = _mm256_add_epi64(mid1, mid2);
  __m256i mid_shift = _mm256_slli_epi64(mid, 32);
  return _mm256_add_epi64(lo, mid_shift);
}

// int64 -> double (vectorized), AVX2-only (no AVX-512VL)
static inline __m256d cvtepi64_pd_avx2(__m256i v64) noexcept {
  alignas(32) long long ei64[4];
  _mm256_store_si256(reinterpret_cast<__m256i*>(ei64), v64);
  int ei32[4] = {
    static_cast<int>(ei64[0]),
    static_cast<int>(ei64[1]),
    static_cast<int>(ei64[2]),
    static_cast<int>(ei64[3])
  };
  __m128i v01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&ei32[0])); // 2x i32
  __m128i v23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&ei32[2])); // 2x i32
  __m128d d01 = _mm_cvtepi32_pd(v01); // -> 2x double
  __m128d d23 = _mm_cvtepi32_pd(v23);
  return _mm256_set_m128d(d23, d01); // [d23 | d01]
}

// range-reduced ln(x): x = m * 2^e, m in [1,2); ln(x) = ln(m) + e*ln2
// ln(m) via 5th-order polynomial around 1: y=m-1 in [0,1)
// range-reduced ln(x): robust to subnormals.
// Normal path: bit-decompose x = m*2^e with m in [1,2), ln(x)=ln(m)+e*ln2
// Subnormal path: fallback to scalar std::log for those lanes only.
static inline __m256d ua_log_rr_pd(__m256d x) noexcept {
  const __m256i mant_mask = _mm256_set1_epi64x(0x000FFFFFFFFFFFFFULL);
  const __m256i exp_mask  = _mm256_set1_epi64x(0x7FF);
  const __m256i exp_1023  = _mm256_set1_epi64x(1023LL);
  const __m256i exp_1023_bits = _mm256_slli_epi64(_mm256_set1_epi64x(1023LL), 52);
  const __m256d one = _mm256_set1_pd(1.0);
  const __m256d ln2 = _mm256_set1_pd(0.693147180559945309417232121458176568);

  __m256i ibits   = _mm256_castpd_si256(x);
  __m256i exp_raw = _mm256_and_si256(_mm256_srli_epi64(ibits, 52), exp_mask);
  __m256i is_sub  = _mm256_cmpeq_epi64(exp_raw, _mm256_setzero_si256()); // subnormal if exp==0

  // normal (non-subnormal) lane math
  __m256i e_i64      = _mm256_sub_epi64(exp_raw, exp_1023); // e = exp - 1023
  __m256i mant_bits  = _mm256_or_si256(_mm256_and_si256(ibits, mant_mask), exp_1023_bits);
  __m256d m          = _mm256_castsi256_pd(mant_bits);      // m in [1,2)
  __m256d y          = _mm256_sub_pd(m, one);
  __m256d y2         = _mm256_mul_pd(y, y);
  __m256d y3         = _mm256_mul_pd(y2, y);
  __m256d y4         = _mm256_mul_pd(y2, y2);
  __m256d y5         = _mm256_mul_pd(y4, y);
  __m256d ln_m_poly  = _mm256_add_pd(y,
                        _mm256_add_pd(_mm256_mul_pd(_mm256_set1_pd(-0.5), y2),
                        _mm256_add_pd(_mm256_mul_pd(_mm256_set1_pd( 1.0/3.0), y3),
                        _mm256_add_pd(_mm256_mul_pd(_mm256_set1_pd(-0.25   ), y4),
                                      _mm256_mul_pd(_mm256_set1_pd( 0.2     ), y5)))));

  // convert e_i64 -> double (AVX2-safe helper from earlier)
  __m256d e = cvtepi64_pd_avx2(e_i64);
  __m256d ln_norm = _mm256_add_pd(ln_m_poly, _mm256_mul_pd(e, ln2));

  // subnormal fallback (scalar std::log for those lanes)
  int submask = _mm256_movemask_pd(_mm256_castsi256_pd(is_sub));
  if (submask != 0) {
    alignas(32) double xv[4], lv[4];
    _mm256_store_pd(xv, x);
    // initialize lv with normal result, then patch sub lanes
    _mm256_store_pd(lv, ln_norm);
    for (int lane = 0; lane < 4; ++lane) {
      if ((submask >> lane) & 1) {
        lv[lane] = std::log(xv[lane]);
      }
    }
    ln_norm = _mm256_load_pd(lv);
  }

  return ln_norm;
}

// add near other helpers at file top (inside the same namespace scope as other helpers)
static inline __m256d ua_sqrt_pd_safe(__m256d x) noexcept {
  // native double sqrt: robust and still fast
  return _mm256_sqrt_pd(x);
}

// ----------------------------------------
// state & core PRNG
// ----------------------------------------
Xoshiro256ssAVX2::Xoshiro256ssAVX2(std::uint64_t seed) noexcept {
  auto sm64 = [](std::uint64_t& v) {
    v += 0x9e3779b97f4a7c15ull;
    std::uint64_t z = v;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
  };
  std::uint64_t x = seed;
  alignas(32) std::uint64_t tmp[16];
  for (int lane = 0; lane < 4; ++lane) {
    tmp[lane + 0]  = sm64(x);
    tmp[lane + 4]  = sm64(x);
    tmp[lane + 8]  = sm64(x);
    tmp[lane + 12] = sm64(x);
  }
  s0 = _mm256_set_epi64x(tmp[12], tmp[8], tmp[4], tmp[0]);
  s1 = _mm256_set_epi64x(tmp[13], tmp[9], tmp[5], tmp[1]);
  s2 = _mm256_set_epi64x(tmp[14], tmp[10], tmp[6], tmp[2]);
  s3 = _mm256_set_epi64x(tmp[15], tmp[11], tmp[7], tmp[3]);
}

__m256i Xoshiro256ssAVX2::next_u64_vec() noexcept {
  __m256i t1 = mullo64(s1, 5u);
  __m256i res = mullo64(rotl64(t1, 7), 9u);
  __m256i t = _mm256_slli_epi64(s1, 17);
  s2 = _mm256_xor_si256(s2, s0);
  s3 = _mm256_xor_si256(s3, s1);
  s1 = _mm256_xor_si256(s1, s2);
  s0 = _mm256_xor_si256(s0, s3);
  s2 = _mm256_xor_si256(s2, t);
  s3 = rotl64(s3, 45);
  return res;
}

void Xoshiro256ssAVX2::generate_u64(std::uint64_t* out, std::size_t n) noexcept {
  std::size_t i = 0;
  while (i + 4 <= n) {
    __m256i v = next_u64_vec();
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(out + i), v);
    i += 4;
  }
  if (i < n) {
    alignas(32) std::uint64_t tmp[4];
    _mm256_store_si256(reinterpret_cast<__m256i*>(tmp), next_u64_vec());
    for (; i < n; ++i) out[i] = tmp[i & 3];
  }
}

void Xoshiro256ssAVX2::generate_double(double* out, std::size_t n) noexcept {
  constexpr std::uint64_t EXP = 0x3FFull << 52;
  std::size_t i = 0;
  while (i + 4 <= n) {
    __m256i u = _mm256_srli_epi64(next_u64_vec(), 12);
    __m256i bits = _mm256_or_si256(u, _mm256_set1_epi64x(EXP));
    __m256d d = _mm256_sub_pd(_mm256_castsi256_pd(bits), _mm256_set1_pd(1.0));
    _mm256_storeu_pd(out + i, d);
    i += 4;
  }
  if (i < n) {
    alignas(32) double tmp[4];
    __m256i u = _mm256_srli_epi64(next_u64_vec(), 12);
    __m256i bits = _mm256_or_si256(u, _mm256_set1_epi64x(EXP));
    __m256d d = _mm256_sub_pd(_mm256_castsi256_pd(bits), _mm256_set1_pd(1.0));
    _mm256_store_pd(tmp, d);
    for (; i < n; ++i) out[i] = tmp[i & 3];
  }
}

// Fully vectorized Marsaglia polar with masked-accept compaction.
// Uses range-reduced ln(s) and NR sqrt for speed & accuracy.
// REPLACE the whole function with this
// k = sqrt( -2*ln(s) / s ) with robust ln + clamping
// REPLACE the whole function with this exact version
void Xoshiro256ssAVX2::generate_normal(double* out, std::size_t n) noexcept {
  std::size_t i = 0;

  const __m256d one   = _mm256_set1_pd(1.0);
  const __m256d zero  = _mm256_set1_pd(0.0);
  const __m256i EXP   = _mm256_set1_epi64x(0x3FFull << 52);
  const __m256d s_min = _mm256_set1_pd(1e-300); // reject ultra-tiny s

  alignas(32) double zu[4], zv[4];

  while (i < n) {
    // two uniforms in (-1,1) via bit tricks (no divides)
    __m256i uu = _mm256_srli_epi64(next_u64_vec(), 12);
    __m256i vv = _mm256_srli_epi64(next_u64_vec(), 12);
    __m256d a  = _mm256_sub_pd(_mm256_castsi256_pd(_mm256_or_si256(uu, EXP)), one);
    __m256d b  = _mm256_sub_pd(_mm256_castsi256_pd(_mm256_or_si256(vv, EXP)), one);
    __m256d u  = _mm256_sub_pd(_mm256_add_pd(a, a), one);
    __m256d v  = _mm256_sub_pd(_mm256_add_pd(b, b), one);

    // s = u^2 + v^2
    __m256d s  = _mm256_add_pd(_mm256_mul_pd(u,u), _mm256_mul_pd(v,v));

    // accept mask: 0 < s < 1 and s >= s_min
    __m256d m_gt0   = _mm256_cmp_pd(s, zero,  _CMP_GT_OQ);
    __m256d m_lt1   = _mm256_cmp_pd(s, one,   _CMP_LT_OQ);
    __m256d m_geMin = _mm256_cmp_pd(s, s_min, _CMP_GT_OQ);
    __m256d m_ok    = _mm256_and_pd(_mm256_and_pd(m_gt0, m_lt1), m_geMin);
    int mask        = _mm256_movemask_pd(m_ok);
    if (mask == 0) continue;

    // safe s (avoid log(0) and division by 0 in rejected lanes)
    __m256d s_safe  = _mm256_blendv_pd(one, s, m_ok);

    // k = sqrt( -2*ln(s) / s ) with robust ln + clamping to avoid NaNs/Infs
    __m256d num     = _mm256_mul_pd(_mm256_set1_pd(-2.0), ua_log_rr_pd(s_safe));
    __m256d frac    = _mm256_div_pd(num, s_safe);
    __m256d fracpos = _mm256_max_pd(frac, _mm256_set1_pd(0.0));
    __m256d fraccl  = _mm256_min_pd(fracpos, _mm256_set1_pd(1e300));
    __m256d k       = ua_sqrt_pd_safe(fraccl);

    // z = u*k , v*k
    __m256d zu_vec = _mm256_mul_pd(u, k);
    __m256d zv_vec = _mm256_mul_pd(v, k);

    // sanitize (zero non-finite lanes), then store to stack
    __m256d ord_u = _mm256_cmp_pd(zu_vec, zu_vec, _CMP_ORD_Q);
    __m256d ord_v = _mm256_cmp_pd(zv_vec, zv_vec, _CMP_ORD_Q);
    zu_vec = _mm256_blendv_pd(_mm256_set1_pd(0.0), zu_vec, ord_u);
    zv_vec = _mm256_blendv_pd(_mm256_set1_pd(0.0), zv_vec, ord_v);

    _mm256_store_pd(zu, zu_vec);
    _mm256_store_pd(zv, zv_vec);

  // compact accepted lanes into the output (no per-sample scalar isfinite)
// we already sanitized zu_vec/zv_vec with vector ORD mask prior to _mm256_store_pd
for (int lane = 0; lane < 4 && i < n; ++lane) {
  if ((mask >> lane) & 1) {
    out[i++] = zu[lane];
    if (i < n) out[i++] = zv[lane];
  }
}

}

}

double Xoshiro256ssAVX2::uniform_scalar() noexcept {
  constexpr std::uint64_t EXP = 0x3FFull << 52;
  alignas(32) std::uint64_t tmp[4];
  _mm256_store_si256(reinterpret_cast<__m256i*>(tmp), _mm256_srli_epi64(next_u64_vec(), 12));
  std::uint64_t bits = tmp[0] | EXP;
  double d; std::memcpy(&d, &bits, sizeof(d)); return d - 1.0;
}

} // namespace ua::detail
