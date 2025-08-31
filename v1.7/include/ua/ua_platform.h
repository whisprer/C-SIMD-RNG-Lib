// v1.6.1/include/ua/ua_platform.h
#pragma once
#if defined(_MSC_VER)
  #define UA_FORCE_INLINE __forceinline
  #define UA_NO_INLINE __declspec(noinline)
#else
  #define UA_FORCE_INLINE inline __attribute__((always_inline))
  #define UA_NO_INLINE __attribute__((noinline))
#endif

#if defined(_MSC_VER)
  #define UA_LIKELY(x)   (x)
  #define UA_UNLIKELY(x) (x)
#else
  #define UA_LIKELY(x)   __builtin_expect(!!(x), 1)
  #define UA_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif
