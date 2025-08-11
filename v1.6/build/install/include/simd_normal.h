// include/simd_normal.h
#pragma once
#include <cstddef>
#include <cstdint>

namespace ua {

// AVX2: consume 'lanes' pairs of 64-bit uniforms and write 'lanes' normals
// lanes must be 4 for AVX2, 8 for AVX-512, 4 for NEON
void simd_normal_avx2(const std::uint64_t* u, const std::uint64_t* v,
                      std::size_t lanes, double mean, double stddev, double* out);

#if defined(UA_ENABLE_AVX512)
void simd_normal_avx512(const std::uint64_t* u, const std::uint64_t* v,
                        std::size_t lanes, double mean, double stddev, double* out);
#endif

#if defined(__aarch64__) || defined(__ARM_NEON)
void simd_normal_neon(const std::uint64_t* u, const std::uint64_t* v,
                      std::size_t lanes, double mean, double stddev, double* out);
#endif

} // namespace ua
