// v1.6.1/include/ua/ua_cpuid.h
#pragma once
#include <cstdint>

namespace ua {

struct CpuFeatures {
  bool sse2{false};
  bool ssse3{false};
  bool avx{false};
  bool avx2{false};
  bool avx512f{false};
  bool fma{false};
};

CpuFeatures query_cpu_features() noexcept;

} // namespace ua
