// D:/code/Universal-Architecture-RNG-Lib/v1.7/src/xoshiro256ss_avx512.cpp
#include "ua/ua_xoshiro256ss_avx512.h"
#include <cmath>
#include <cstring>

namespace ua::detail {

// ----------------------------------------
// helpers
// ----------------------------------------
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

// range-reduced ln(x): x = m * 2^e, m in [1,2); ln(x) = ln(m) + e*ln2
static inline __m512d ua_log_rr_pd(__m512d x) noexcept {
  const __m512i mant_mask = _mm512_set1_epi64((long long)0x000FFFFFFFFFFFFFULL);
  const __m512i exp_mask  = _mm512_set1_epi64(0x7FF);
  const __m512i exp_1023  = _mm512_set1_epi64(1023);
  const __m512i exp_1023_bits = _mm512_slli_epi64(_mm512_set1_epi64(1023), 52);
  const __m512d one = _mm512_set1_pd(1.0);
  const __m512d ln2 = _mm512_set1_pd(0.693147180559945309417232121458176568);

  __m512i ibits   = _mm512_castpd_si512(x);
  __m512i exp_raw = _mm512_and_si512(_mm512_srli_epi64(ibits, 52), exp_mask);
  __mmask8 m_sub  = _mm512_cmpeq_epi64_mask(exp_raw, _mm512_setzero_si512());

  __m512i e_i64     = _mm512_sub_epi64(exp_raw, exp_1023);
  __m512i mant_bits = _mm512_or_si512(_mm512_and_si512(ibits, mant_mask), exp_1023_bits);
  __m512d m         = _mm512_castsi512_pd(mant_bits);
  __m512d y         = _mm512_sub_pd(m, one);
  __m512d y2        = _mm512_mul_pd(y, y);
  __m512d y3        = _mm512_mul_pd(y2, y);
  __m512d y4        = _mm512_mul_pd(y2, y2);
  __m512d y5        = _mm512_mul_pd(y4, y);
  __m512d ln_m_poly = _mm512_add_pd(y,
                         _mm512_add_pd(_mm512_mul_pd(_mm512_set1_pd(-0.5), y2),
                         _mm512_add_pd(_mm512_mul_pd(_mm512_set1_pd( 1.0/3.0), y3),
                         _mm512_add_pd(_mm512_mul_pd(_mm512_set1_pd(-0.25   ), y4),
                                       _mm512_mul_pd(_mm512_set1_pd( 0.2     ), y5)))));

  __m512d e      = _mm512_cvtepi64_pd(e_i64);
  __m512d ln_norm = _mm512_add_pd(ln_m_poly, _mm512_mul_pd(e, ln2));

  if (m_sub) {
    alignas(64) double xv[8], lv[8];
    _mm512_store_pd(xv, x);
    _mm512_store_pd(lv, ln_norm);
    for (int lane = 0; lane < 8; ++lane) {
      if ((m_sub >> lane) & 1) {
        lv[lane] = std::log(xv[lane]);
      }
    }
    ln_norm = _mm512_load_pd(lv);
  }

  return ln_norm;
}

// add near other helpers at file top (inside same namespace scope)
static inline __m512d ua_sqrt_pd_safe(__m512d x) noexcept {
  return _mm512_sqrt_pd(x);
}

// ----------------------------------------
// state & core PRNG
// ----------------------------------------
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

// Fully vectorized Marsaglia polar with masked-accept + range-reduced log & NR sqrt.
// REPLACE the whole function with this
// REPLACE the whole function with this exact version
void Xoshiro256ssAVX512::generate_normal(double* out, std::size_t n) noexcept {
  std::size_t i = 0;

  const __m512d one   = _mm512_set1_pd(1.0);
  const __m512d zero  = _mm512_set1_pd(0.0);
  const __m512i EXP   = _mm512_set1_epi64((long long)(0x3FFull << 52));
  const __m512d s_min = _mm512_set1_pd(1e-300); // reject ultra-tiny s

  alignas(64) double zu[8], zv[8];

  while (i < n) {
    // two uniforms in (-1,1) via bit tricks (no divides)
    __m512i uu = _mm512_srli_epi64(next_u64_vec(), 12);
    __m512i vv = _mm512_srli_epi64(next_u64_vec(), 12);
    __m512d a  = _mm512_sub_pd(_mm512_castsi512_pd(_mm512_or_epi64(uu, EXP)), one);
    __m512d b  = _mm512_sub_pd(_mm512_castsi512_pd(_mm512_or_epi64(vv, EXP)), one);
    __m512d u  = _mm512_sub_pd(_mm512_add_pd(a, a), one);
    __m512d v  = _mm512_sub_pd(_mm512_add_pd(b, b), one);

    // s = u^2 + v^2
    __m512d s  = _mm512_add_pd(_mm512_mul_pd(u,u), _mm512_mul_pd(v,v));

    // accept mask: 0 < s < 1 and s >= s_min
    __mmask8 m_ok = _mm512_kand(
        _mm512_kand(_mm512_cmp_pd_mask(s, zero, _CMP_GT_OQ),
                    _mm512_cmp_pd_mask(s, one,  _CMP_LT_OQ)),
        _mm512_cmp_pd_mask(s, s_min, _CMP_GT_OQ));

    if (m_ok == 0) continue;

    // safe s for rejected lanes
    __m512d s_safe = _mm512_mask_mov_pd(one, m_ok, s);

    // k = sqrt( -2*ln(s) / s ) with robust ln + clamping
    __m512d num     = _mm512_mul_pd(_mm512_set1_pd(-2.0), ua_log_rr_pd(s_safe));
    __m512d frac    = _mm512_div_pd(num, s_safe);
    __m512d fracpos = _mm512_max_pd(frac, _mm512_set1_pd(0.0));
    __m512d fraccl  = _mm512_min_pd(fracpos, _mm512_set1_pd(1e300));
    __m512d k       = ua_sqrt_pd_safe(fraccl);

    __m512d zu_vec = _mm512_mul_pd(u, k);
    __m512d zv_vec = _mm512_mul_pd(v, k);

    // sanitize (zero non-finite lanes), then store to stack
    __mmask8 ord_u = _mm512_cmp_pd_mask(zu_vec, zu_vec, _CMP_ORD_Q);
    __mmask8 ord_v = _mm512_cmp_pd_mask(zv_vec, zv_vec, _CMP_ORD_Q);
    zu_vec = _mm512_mask_mov_pd(_mm512_set1_pd(0.0), ord_u, zu_vec);
    zv_vec = _mm512_mask_mov_pd(_mm512_set1_pd(0.0), ord_v, zv_vec);

    _mm512_store_pd(zu, zu_vec);
    _mm512_store_pd(zv, zv_vec);

for (int lane = 0; lane < 8 && i < n; ++lane) {
  if ((m_ok >> lane) & 1) {
    out[i++] = zu[lane];
    if (i < n) out[i++] = zv[lane];
  }
}

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
