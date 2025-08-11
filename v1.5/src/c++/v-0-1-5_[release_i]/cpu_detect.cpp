#include "cpu_detect.h"

#ifdef _MSC_VER
    #include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
    #include <cpuid.h>
#endif

extern "C" {

bool detect_avx512_support() {
    #if defined(__x86_64__) || defined(_M_X64)
        #ifdef _MSC_VER
            // MSVC approach
            int cpuInfo[4];
            int cpuInfo7[4] = {0};
            
            // Check for OSXSAVE (required for AVX support)
            __cpuid(cpuInfo, 1);
            bool osxsave = (cpuInfo[2] & (1 << 27)) != 0;
            if (!osxsave) return false;
            
            // Check for AVX-512 Foundation
            __cpuidex(cpuInfo7, 7, 0);
            bool avx512f = (cpuInfo7[1] & (1 << 16)) != 0;
            
            // Check if OS supports AVX-512 state
            unsigned long long xcr0 = _xgetbv(0);
            bool osSupport = (xcr0 & 0xE0) == 0xE0; // Check ZMM registers support
            
            return avx512f && osSupport;
        #else
            // GCC/Clang approach
            #ifdef USE_AVX512
            unsigned int eax, ebx, ecx, edx;
            
            // Check for CPUID leaf 1 support (OSXSAVE)
            if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
                bool osxsave = (ecx & (1 << 27)) != 0;
                if (!osxsave) return false;
                
                // Check for AVX-512 Foundation
                if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
                    bool avx512f = (ebx & (1 << 16)) != 0;
                    
                    // Check OS support via XCR0
                    if (avx512f) {
                        unsigned int xcr0_lo, xcr0_hi;
                        __asm__("xgetbv" : "=a"(xcr0_lo), "=d"(xcr0_hi) : "c"(0));
                        unsigned long long xcr0 = ((unsigned long long)xcr0_hi << 32) | xcr0_lo;
                        bool osSupport = (xcr0 & 0xE0) == 0xE0;
                        return osSupport;
                    }
                }
            }
            #endif
            return false;
        #endif
    #else
        return false;
    #endif
}

bool detect_avx2_support() {
    #if defined(__x86_64__) || defined(_M_X64)
        #ifdef _MSC_VER
            // MSVC approach
            int cpuInfo[4];
            int cpuInfo7[4] = {0};
            
            // Check for OSXSAVE (required for AVX/AVX2 support)
            __cpuid(cpuInfo, 1);
            bool osxsave = (cpuInfo[2] & (1 << 27)) != 0;
            bool avx = (cpuInfo[2] & (1 << 28)) != 0;
            if (!osxsave || !avx) return false;
            
            // Check for AVX2
            __cpuidex(cpuInfo7, 7, 0);
            bool avx2 = (cpuInfo7[1] & (1 << 5)) != 0;
            
            // Check if OS supports AVX state
            unsigned long long xcr0 = _xgetbv(0);
            bool osSupport = (xcr0 & 0x6) == 0x6; // Check YMM registers support
            
            return avx2 && osSupport;
        #else
            // GCC/Clang approach
            #ifdef USE_AVX2
            unsigned int eax, ebx, ecx, edx;
            
            // Check for CPUID leaf 1 support (OSXSAVE & AVX)
            if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
                bool osxsave = (ecx & (1 << 27)) != 0;
                bool avx = (ecx & (1 << 28)) != 0;
                if (!osxsave || !avx) return false;
                
                // Check for AVX2
                if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
                    bool avx2 = (ebx & (1 << 5)) != 0;
                    
                    // Check OS support via XCR0
                    if (avx2) {
                        unsigned int xcr0_lo, xcr0_hi;
                        __asm__("xgetbv" : "=a"(xcr0_lo), "=d"(xcr0_hi) : "c"(0));
                        unsigned long long xcr0 = ((unsigned long long)xcr0_hi << 32) | xcr0_lo;
                        bool osSupport = (xcr0 & 0x6) == 0x6;
                        return osSupport;
                    }
                }
            }
            #endif
            return false;
        #endif
    #else
        return false;
    #endif
}

bool detect_sse2_support() {
    #if defined(__x86_64__) || defined(_M_X64)
        // All x86-64 CPUs support SSE2, so we can just return true
        return true;
    #else
        #ifdef _MSC_VER
            // MSVC approach for x86
            int cpuInfo[4];
            __cpuid(cpuInfo, 1);
            return (cpuInfo[3] & (1 << 26)) != 0;
        #else
            // GCC/Clang approach for x86
            unsigned int eax, ebx, ecx, edx;
            if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
                return (edx & (1 << 26)) != 0;
            }
            return false;
        #endif
    #endif
}

} // extern "C"