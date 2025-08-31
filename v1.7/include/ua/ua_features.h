#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace ua {

enum class Isa : uint8_t { Scalar=0, SSE2, AVX, AVX2, AVX512 };

inline Isa parse_force_isa_env() {
  const char* v = std::getenv("UA_RNG_FORCE_ISA");
  if (!v) return Isa::Scalar; // means "no override"
  if (std::strcmp(v,"scalar")==0) return Isa::Scalar;
  if (std::strcmp(v,"sse2")==0)   return Isa::SSE2;
  if (std::strcmp(v,"avx")==0)    return Isa::AVX;
  if (std::strcmp(v,"avx2")==0)   return Isa::AVX2;
  if (std::strcmp(v,"avx512")==0) return Isa::AVX512;
  return Isa::Scalar;
}

} // namespace ua
