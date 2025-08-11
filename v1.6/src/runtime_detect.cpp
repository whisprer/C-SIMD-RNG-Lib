// src/runtime_detect.cpp
#include <cstdint>
#include "runtime_detect.h"

#if defined(_MSC_VER)
  #include <intrin.h>
#endif

namespace ua {

static inline void cpuid_ex(int info[4], int eax, int ecx) noexcept {
#if defined(_MSC_VER)
  __cpuidex(info, eax, ecx);
#elif defined(__GNUC__) || defined(__clang__)
  int a,b,c,d;
  __asm__ volatile ("cpuid"
                    : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                    : "a"(eax), "c"(ecx));
  info[0]=a; info[1]=b; info[2]=c; info[3]=d;
#else
  info[0]=info[1]=info[2]=info[3]=0;
#endif
}

static inline std::uint64_t xgetbv0() noexcept {
#if defined(_MSC_VER)
  return _xgetbv(0);
#elif defined(__GNUC__) || defined(__clang__)
  std::uint32_t eax, edx;
  __asm__ volatile (".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(0));
  return (static_cast<std::uint64_t>(edx) << 32) | eax;
#else
  return 0;
#endif
}

SimdTier detect_simd_tier() noexcept {
#if defined(__aarch64__) || defined(__ARM_NEON)
  return SimdTier::NEON;
#endif

  // Default
  SimdTier tier = SimdTier::Scalar;

  // x86/x64 path
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  int info1[4] = {0};
  cpuid_ex(info1, 1, 0);
  const bool osxsave = (info1[2] & (1 << 27)) != 0;  // ECX.OSXSAVE

  std::uint64_t xcr0 = 0;
  if (osxsave) xcr0 = xgetbv0();

  // For AVX/AVX2 we need XMM (bit 1) and YMM (bit 2)
  const bool os_avx   = osxsave && ((xcr0 & 0x6) == 0x6);

  int info7[4] = {0};
  cpuid_ex(info7, 7, 0); // leaf 7 subleaf 0

  const bool hw_avx2     = (info7[1] & (1 << 5))  != 0;   // EBX.AVX2
  const bool hw_avx512f  = (info7[1] & (1 << 16)) != 0;   // EBX.AVX512F
  const bool hw_avx512dq = (info7[1] & (1 << 17)) != 0;   // EBX.AVX512DQ

  // For AVX-512 we also need opmask/ZMM state: XCR0 bits 5,6,7
  const bool os_avx512 = osxsave && ((xcr0 & 0xE0) == 0xE0) && ((xcr0 & 0x6) == 0x6);

  // Prefer AVX-512 when compiled and truly available
#if defined(UA_ENABLE_AVX512)
  if (os_avx512 && hw_avx512f && hw_avx512dq) return SimdTier::AVX512;
#endif
#if defined(UA_ENABLE_AVX2)
  if (os_avx && hw_avx2) return SimdTier::AVX2;
#endif
#endif

  return tier;
}

} // namespace ua
