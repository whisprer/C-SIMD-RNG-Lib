// v1.6.1/include/ua/ua_cpuid.h
#pragma once
#include <cstdint>
#if defined(_MSC_VER)
  #include <intrin.h>
#endif

namespace ua {

struct CpuFeatures {
  bool sse2{false};
  bool ssse3{false};
  bool avx{false};
  bool avx2{false};
  bool avx512f{false};
  bool fma{false};
};

inline CpuFeatures detect_cpu_features() {
  CpuFeatures f{};
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  int regs[4] = {0,0,0,0};
# if defined(_MSC_VER)
  __cpuid(regs, 1);
  int ecx1 = regs[2], edx1 = regs[3];
  f.sse2  = (edx1 & (1<<26)) != 0;
  f.ssse3 = (ecx1 & (1<<9))  != 0;
  bool osxsave = (ecx1 & (1<<27)) != 0;
  f.fma   = (ecx1 & (1<<12)) != 0;
  bool ymm_ok = false;
  if (osxsave) {
    unsigned long long xcr = _xgetbv(0);
    ymm_ok = (xcr & 0x6) == 0x6;
  }
  f.avx = ((ecx1 & (1<<28)) != 0) && ymm_ok;
  __cpuidex(regs, 7, 0);
  int ebx7 = regs[1];
  f.avx2     = f.avx && ((ebx7 & (1<<5)) != 0);
  f.avx512f  = ((ebx7 & (1<<16)) != 0) && (( _xgetbv(0) & 0xE6) == 0xE6);
# else
  unsigned int eax, ebx, ecx, edx;
  __asm__ __volatile__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1));
  f.sse2  = (edx & (1u<<26)) != 0;
  f.ssse3 = (ecx & (1u<<9))  != 0;
  bool osxsave = (ecx & (1u<<27)) != 0;
  f.fma   = (ecx & (1u<<12)) != 0;
  bool ymm_ok = false;
  if (osxsave) {
    unsigned int xcr0_eax, xcr0_edx;
    __asm__ __volatile__("xgetbv" : "=a"(xcr0_eax), "=d"(xcr0_edx) : "c"(0));
    ymm_ok = (xcr0_eax & 0x6) == 0x6;
  }
  f.avx = ((ecx & (1u<<28)) != 0) && ymm_ok;
  __asm__ __volatile__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(7), "c"(0));
  f.avx2     = f.avx && ((ebx & (1u<<5)) != 0);
  bool zmm_ok = false;
  if (osxsave) {
    unsigned int xcr0_eax, xcr0_edx;
    __asm__ __volatile__("xgetbv" : "=a"(xcr0_eax), "=d"(xcr0_edx) : "c"(0));
    zmm_ok = ( (xcr0_eax & 0xE6) == 0xE6 );
  }
  f.avx512f  = zmm_ok && ((ebx & (1u<<16)) != 0);
# endif
#endif
  return f;
}

} // namespace ua
