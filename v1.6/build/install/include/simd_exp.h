#pragma once
#include <cstdint>
#include <cstddef>

// Vectorized exp(-0.5 * x^2) for double lanes.
// Used only in Ziggurat wedge accept; accuracy target ~1e-7 rel.

namespace ua {

// --------------------------- AVX2 ---------------------------
#if defined(UA_ENABLE_AVX2)
#include <immintrin.h>
static inline __m256d ua_exp_neg_half_x2_avx2(__m256d x) noexcept {
  const __m256d c_mhalf = _mm256_set1_pd(-0.5);
  const __m256d c_log2e = _mm256_set1_pd(1.44269504088896340736);
  const __m256d c_ln2   = _mm256_set1_pd(0.69314718055994530942);

  __m256d y = _mm256_mul_pd(c_mhalf, _mm256_mul_pd(x, x));
  y = _mm256_max_pd(_mm256_set1_pd(-50.0), _mm256_min_pd(_mm256_set1_pd(50.0), y));

  __m256d a   = _mm256_mul_pd(y, c_log2e);
  __m256d n_d = _mm256_round_pd(a, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  __m256d f   = _mm256_sub_pd(a, n_d);
  __m256d g   = _mm256_mul_pd(f, c_ln2);

  __m256d g2 = _mm256_mul_pd(g, g);
  __m256d g3 = _mm256_mul_pd(g2, g);
  __m256d g4 = _mm256_mul_pd(g2, g2);
  __m256d g5 = _mm256_mul_pd(g4, g);
  __m256d g6 = _mm256_mul_pd(g4, g2);

  __m256d p = _mm256_add_pd(_mm256_set1_pd(1.0), g);
  p = _mm256_add_pd(p, _mm256_mul_pd(_mm256_set1_pd(0.5),      g2));
  p = _mm256_add_pd(p, _mm256_mul_pd(_mm256_set1_pd(1.0/6.0),  g3));
  p = _mm256_add_pd(p, _mm256_mul_pd(_mm256_set1_pd(1.0/24.0), g4));
  p = _mm256_add_pd(p, _mm256_mul_pd(_mm256_set1_pd(1.0/120.0),g5));
  p = _mm256_add_pd(p, _mm256_mul_pd(_mm256_set1_pd(1.0/720.0),g6));

  // 2^n by crafting exponent bits per lane
  alignas(32) double nbias[4];
  _mm256_storeu_pd(nbias, _mm256_add_pd(n_d, _mm256_set1_pd(1023.0)));
  alignas(32) std::uint64_t bits[4];
  for (int i=0;i<4;++i) {
    long long nll = llround(nbias[i]); // nearest int
    bits[i] = (std::uint64_t(nll) << 52);
  }
  __m256i two_n_bits = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(bits));
  __m256d two_n = _mm256_castsi256_pd(two_n_bits);

  return _mm256_mul_pd(p, two_n);
}
#endif

// --------------------------- AVX-512 ---------------------------
#if defined(UA_ENABLE_AVX512)
#include <immintrin.h>
static inline __m512d ua_exp_neg_half_x2_avx512(__m512d x) noexcept {
  const __m512d c_mhalf = _mm512_set1_pd(-0.5);
  const __m512d c_log2e = _mm512_set1_pd(1.44269504088896340736);
  const __m512d c_ln2   = _mm512_set1_pd(0.69314718055994530942);

  __m512d y = _mm512_mul_pd(c_mhalf, _mm512_mul_pd(x, x));
  y = _mm512_max_pd(_mm512_set1_pd(-50.0), _mm512_min_pd(_mm512_set1_pd(50.0), y));

  __m512d a   = _mm512_mul_pd(y, c_log2e);
  __m512d n_d = _mm512_roundscale_pd(a, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  __m512d f   = _mm512_sub_pd(a, n_d);
  __m512d g   = _mm512_mul_pd(f, c_ln2);

  __m512d g2 = _mm512_mul_pd(g, g);
  __m512d g3 = _mm512_mul_pd(g2, g);
  __m512d g4 = _mm512_mul_pd(g2, g2);
  __m512d g5 = _mm512_mul_pd(g4, g);
  __m512d g6 = _mm512_mul_pd(g4, g2);

  __m512d p = _mm512_add_pd(_mm512_set1_pd(1.0), g);
  p = _mm512_add_pd(p, _mm512_mul_pd(_mm512_set1_pd(0.5),      g2));
  p = _mm512_add_pd(p, _mm512_mul_pd(_mm512_set1_pd(1.0/6.0),  g3));
  p = _mm512_add_pd(p, _mm512_mul_pd(_mm512_set1_pd(1.0/24.0), g4));
  p = _mm512_add_pd(p, _mm512_mul_pd(_mm512_set1_pd(1.0/120.0),g5));
  p = _mm512_add_pd(p, _mm512_mul_pd(_mm512_set1_pd(1.0/720.0),g6));

  alignas(64) double nbias[8];
  _mm512_storeu_pd(nbias, _mm512_add_pd(n_d, _mm512_set1_pd(1023.0)));
  alignas(64) std::uint64_t bits[8];
  for (int i=0;i<8;++i) {
    long long nll = llround(nbias[i]);
    bits[i] = (std::uint64_t(nll) << 52);
  }
  __m512i two_n_bits = _mm512_loadu_si512(reinterpret_cast<const void*>(bits));
  __m512d two_n = _mm512_castsi512_pd(two_n_bits);

  return _mm512_mul_pd(p, two_n);
}
#endif

// --------------------------- NEON (AArch64) ---------------------------
#if defined(__aarch64__) || defined(__ARM_NEON)
#include <arm_neon.h>
static inline float64x2_t ua_exp_neg_half_x2_neon(float64x2_t x) noexcept {
  float64x2_t y = vmulq_f64(vdupq_n_f64(-0.5), vmulq_f64(x, x));
  y = vmaxq_f64(vdupq_n_f64(-50.0), vminq_f64(vdupq_n_f64(50.0), y));

  const float64x2_t log2e = vdupq_n_f64(1.44269504088896340736);
  float64x2_t a = vmulq_f64(y, log2e);
  float64x2_t n_d = vrndnq_f64(a);
  float64x2_t f   = vsubq_f64(a, n_d);
  const float64x2_t ln2 = vdupq_n_f64(0.69314718055994530942);
  float64x2_t g = vmulq_f64(f, ln2);

  float64x2_t g2 = vmulq_f64(g, g);
  float64x2_t g3 = vmulq_f64(g2, g);
  float64x2_t g4 = vmulq_f64(g2, g2);
  float64x2_t g5 = vmulq_f64(g4, g);
  float64x2_t g6 = vmulq_f64(g4, g2);

  float64x2_t p = vaddq_f64(vdupq_n_f64(1.0), g);
  p = vaddq_f64(p, vmulq_f64(vdupq_n_f64(0.5),      g2));
  p = vaddq_f64(p, vmulq_f64(vdupq_n_f64(1.0/6.0),  g3));
  p = vaddq_f64(p, vmulq_f64(vdupq_n_f64(1.0/24.0), g4));
  p = vaddq_f64(p, vmulq_f64(vdupq_n_f64(1.0/120.0),g5));
  p = vaddq_f64(p, vmulq_f64(vdupq_n_f64(1.0/720.0),g6));

  double nbias[2];
  vst1q_f64(nbias, vaddq_f64(n_d, vdupq_n_f64(1023.0)));
  std::uint64_t bits[2] = {
    (std::uint64_t((long long)llround(nbias[0])) << 52),
    (std::uint64_t((long long)llround(nbias[1])) << 52)
  };
  float64x2_t two_n = vreinterpretq_f64_u64(vld1q_u64(bits));
  return vmulq_f64(p, two_n);
}
#endif

} // namespace ua
