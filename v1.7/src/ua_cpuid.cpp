// D:/code/Universal-Architecture-RNG-Lib/v1.7/src/ua_cpuid.cpp
#include "ua/ua_cpuid.h"

#if defined(_MSC_VER)
  #include <intrin.h>
#else
  #include <cpuid.h>
#endif

namespace ua {

static inline unsigned long long xgetbv0() {
#if defined(_MSC_VER)
  return _xgetbv(0);
#elif defined(__GNUC__) || defined(__clang__)
  unsigned int eax=0, edx=0;
  // xgetbv with ecx=0 (XCR0). Use raw opcode for broad GCC/MinGW coverage.
  __asm__ __volatile__ (".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(0));
  return (static_cast<unsigned long long>(edx) << 32) | eax;
#else
  return 0ull;
#endif
}

static void cpuid_ex(unsigned int leaf, unsigned int subleaf, unsigned int regs[4]) {
#if defined(_MSC_VER)
  int r[4]; __cpuidex(r, static_cast<int>(leaf), static_cast<int>(subleaf));
  regs[0] = static_cast<unsigned>(r[0]);
  regs[1] = static_cast<unsigned>(r[1]);
  regs[2] = static_cast<unsigned>(r[2]);
  regs[3] = static_cast<unsigned>(r[3]);
#else
  unsigned int a, b, c, d;
  __cpuid_count(leaf, subleaf, a, b, c, d);
  regs[0] = a; regs[1] = b; regs[2] = c; regs[3] = d;
#endif
}

CpuFeatures query_cpu_features() noexcept {
  CpuFeatures f{};
  unsigned int r[4]{};

  // Basic leaf
  cpuid_ex(0, 0, r);
  unsigned int max_leaf = r[0];
  if (max_leaf < 1) return f;

  // Leaf 1: SSE2/OSXSAVE/AVX/FMA
  cpuid_ex(1, 0, r);
  const unsigned ecx = r[2];
  const unsigned edx = r[3];
  const bool osxsave = (ecx & (1u << 27)) != 0;
  const bool avx_bit = (ecx & (1u << 28)) != 0;

  f.sse2  = (edx & (1u << 26)) != 0;
  f.ssse3 = (ecx & (1u <<  9)) != 0;
  f.fma   = (ecx & (1u << 12)) != 0;

  bool os_avx_ok = false;
  bool os_avx512_ok = false;
  if (osxsave) {
    const unsigned long long xcr0 = xgetbv0();
    // need XMM (bit1) + YMM (bit2) for AVX/AVX2
    os_avx_ok = ( (xcr0 & 0x6ull) == 0x6ull );
    // need XMM+YMM plus Opmask(5), ZMM_hi256(6), Hi16_ZMM(7) for AVX512
    os_avx512_ok = ( (xcr0 & 0xE6ull) == 0xE6ull );
  }

  // Leaf 7: AVX2/AVX512F
  if (max_leaf >= 7) {
    cpuid_ex(7, 0, r);
    const unsigned ebx = r[1];
    const bool avx2_bit    = (ebx & (1u << 5))  != 0;
    const bool avx512f_bit = (ebx & (1u << 16)) != 0;

    if (avx_bit && os_avx_ok) {
      f.avx = true;
      if (avx2_bit)    f.avx2    = true;
      if (avx512f_bit && os_avx512_ok) f.avx512f = true;
    }
  }

  return f;
}

} // namespace ua
