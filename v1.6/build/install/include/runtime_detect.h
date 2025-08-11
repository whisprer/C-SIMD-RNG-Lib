#pragma once
#include <cstdint>

namespace ua {

enum class SimdTier : std::uint8_t { Scalar=0, AVX2=1, AVX512=2, NEON=3 };

SimdTier detect_simd_tier() noexcept;

} // namespace ua
