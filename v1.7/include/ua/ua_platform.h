#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <type_traits>

#if defined(_MSC_VER)
  #define UA_FORCE_INLINE __forceinline
  #include <malloc.h>
#else
  #define UA_FORCE_INLINE __attribute__((always_inline)) inline
  #include <mm_malloc.h>
#endif

namespace ua {

UA_FORCE_INLINE void* aligned_malloc_bytes(std::size_t n, std::size_t align) {
#if defined(_MSC_VER)
  return _aligned_malloc(n, align);
#else
  return _mm_malloc(n, align);
#endif
}

UA_FORCE_INLINE void aligned_free(void* p) {
#if defined(_MSC_VER)
  _aligned_free(p);
#else
  _mm_free(p);
#endif
}

template<class T>
UA_FORCE_INLINE T* aligned_malloc(std::size_t count, std::size_t align) {
  void* p = aligned_malloc_bytes(sizeof(T)*count, align);
  if (!p) throw std::bad_alloc();
  return reinterpret_cast<T*>(p);
}

} // namespace ua
