#pragma once

#ifndef UA_STREAM_STORES
#define UA_STREAM_STORES 0
#endif

namespace ua {
enum class SimdTier : int {
  Scalar    = 0,
  AVX2      = 2,
  AVX512F   = 5
};
} // namespace ua
