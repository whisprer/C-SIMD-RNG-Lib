// src/simd_normal_avx2.cpp
#include <immintrin.h>
#include <cstdint>
#include <cstddef>
#include <cmath>

namespace ua {

// Convert 64-bit uniform to double in (0,1); add a tiny bias to avoid log(0)
static inline __m256d u64_to_unit_double_avx2(__m256i u) {
  const __m256i exp_one = _mm256_set1_epi64x(0x3FF0000000000000ULL);
  __m256i mant = _mm256_srli_epi64(u, 12);
  __m256i bits = _mm256_or_si256(exp_one, mant);
  __m256d d    = _mm256_castsi256_pd(bits);
  __m256d x    = _mm256_sub_pd(d, _mm256_set1_pd(1.0));
  return _mm256_max_pd(x, _mm256_set1_pd(0x1.0p-54));
}

static inline void box_muller_scalar4(const double U[4], const double V[4],
                                      double mean, double stddev,
                                      double out4[4]) {
  constexpr double TwoPi = 6.2831853071795864769;
  for (int i = 0; i < 4; ++i) {
    double u = U[i];
    double v = V[i];
    double r  = std::sqrt(-2.0 * std::log(u));
    double th = TwoPi * v;
    double s = std::sin(th);
    double c = std::cos(th);
    out4[i] = mean + stddev * ((i & 1) ? r * s : r * c);
  }
}

void simd_normal_avx2(const std::uint64_t* u, const std::uint64_t* v,
                      std::size_t lanes, double mean, double stddev, double* out)
{
  const std::size_t step = 4;
  for (std::size_t i = 0; i < lanes; i += step) {
    __m256i U64 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(u + i));
    __m256i V64 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(v + i));
    __m256d Uv  = u64_to_unit_double_avx2(U64);
    __m256d Vv  = u64_to_unit_double_avx2(V64);

    alignas(32) double Ud[4], Vd[4], Z[4];
    _mm256_storeu_pd(Ud, Uv);
    _mm256_storeu_pd(Vd, Vv);

    box_muller_scalar4(Ud, Vd, mean, stddev, Z);
    _mm256_storeu_pd(out + i, _mm256_loadu_pd(Z));
  }
}

} // namespace ua
