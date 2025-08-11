// include/universal_rng.h
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include "ua_export.h"
#include "runtime_detect.h"  // ua::SimdTier

namespace ua {

// Which core generator to use
enum class Algorithm : std::uint8_t {
  Xoshiro256ss = 0,
  Philox4x32_10 = 1
};

// Init parameters (RAII-friendly)
struct Init {
  Algorithm     algo = Algorithm::Xoshiro256ss;
  std::uint64_t seed = 0xDEADBEEFCAFEBABEULL;
  std::uint64_t stream = 0;

  struct {
    std::size_t capacity_u64 = 8192; // internal buffer capacity in u64s
  } buffer;
};

// Forward-declare the internal context (defined in src/universal_rng.cpp)
class GeneratorContext;

class UA_API UniversalRng {
public:
  explicit UniversalRng(const Init& init);
  ~UniversalRng();

  // Core outputs
  void generate_u64(std::uint64_t* out, std::size_t n);
  void generate_double(double* out, std::size_t n);                  // [0,1)
  void generate_normal(double mean, double stddev, double* out, std::size_t n);

  // Inspect the selected SIMD tier at runtime (for benchmarking/logging)
  ua::SimdTier simd_tier() const noexcept;

private:
  std::unique_ptr<GeneratorContext> ctx_;
};

} // namespace ua
