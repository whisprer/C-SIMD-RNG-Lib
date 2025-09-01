#pragma once
#include <immintrin.h>
#include <cstddef>
#include <cstdint>

namespace ua::detail {

struct Xoshiro256ssAVX512 {
  Xoshiro256ssAVX512() = delete;
  explicit Xoshiro256ssAVX512(std::uint64_t seed) noexcept;

  void generate_u64(std::uint64_t* out, std::size_t n) noexcept;
  void generate_double(double* out, std::size_t n) noexcept;  // [0,1)
  void generate_normal(double* out, std::size_t n) noexcept;  // N(0,1)

private:
  __m512i s0, s1, s2, s3;

  __m512i next_u64_vec() noexcept;
  double  uniform_scalar() noexcept;
};

} // namespace ua::detail
