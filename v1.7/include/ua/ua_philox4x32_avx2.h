#pragma once
#include <cstdint>
#include <cstddef>
#include <immintrin.h>

namespace ua {

// Philox4x32-10 constants
static constexpr uint32_t PH_M0 = 0xD2511F53u;
static constexpr uint32_t PH_M1 = 0xCD9E8D57u;
static constexpr uint32_t PH_W0 = 0x9E3779B9u;
static constexpr uint32_t PH_W1 = 0xBB67AE85u;

struct Philox4x32AVX2 {
  __m256i c0, c1, c2, c3;   // counters
  __m256i k0, k1;           // keys

  static inline __m256i add32(__m256i a, __m256i b){ return _mm256_add_epi32(a,b); }
  static inline __m256i xor32(__m256i a, __m256i b){ return _mm256_xor_si256(a,b); }

  static inline __m256i mullo_const_u32(__m256i x, uint32_t m){
    return _mm256_mullo_epi32(x, _mm256_set1_epi32((int)m));
  }
  static inline __m256i mulhi_const_u32(__m256i x, uint32_t m){
    const __m256i M = _mm256_set1_epi32((int)m);
    __m256i even = _mm256_mul_epu32(x, M);
    __m256i even_hi = _mm256_srli_epi64(even, 32);
    __m256i oddx = _mm256_srli_epi64(x, 32);
    __m256i odd  = _mm256_mul_epu32(oddx, M);
    __m256i odd_hi  = _mm256_srli_epi64(odd, 32);
    __m256i even32 = _mm256_and_si256(even_hi, _mm256_set1_epi64x(0x00000000FFFFFFFFull));
    __m256i odd32  = _mm256_slli_epi64(odd_hi, 32);
    return _mm256_or_si256(even32, odd32);
  }
  static inline __m256i pack_u64(__m256i hi32, __m256i lo32){
    __m256i hi = _mm256_slli_epi64(hi32, 32);
    __m256i lo = _mm256_and_si256(lo32, _mm256_set1_epi64x(0x00000000FFFFFFFFull));
    return _mm256_or_si256(hi, lo);
  }

  static inline uint64_t sm64(uint64_t& x){
    x += 0x9e3779b97f4a7c15ull;
    uint64_t z = x;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
  }

  explicit Philox4x32AVX2(uint64_t seed){
    uint64_t z = seed;
    uint32_t k0s = (uint32_t)sm64(z);
    uint32_t k1s = (uint32_t)sm64(z);
    k0 = _mm256_set1_epi32((int)k0s);
    k1 = _mm256_set1_epi32((int)k1s);
    __m256i lane = _mm256_set_epi32(7,6,5,4,3,2,1,0);
    c0 = lane; c1 = _mm256_setzero_si256(); c2 = _mm256_setzero_si256(); c3 = _mm256_setzero_si256();
  }

  inline void bump_nocarry(int inc){ c0 = _mm256_add_epi32(c0, _mm256_set1_epi32(inc)); }

  static inline void two_rounds(__m256i& r0,__m256i& r1,__m256i& r2,__m256i& r3,
                                __m256i& kk0,__m256i& kk1,
                                const __m256i& W0v,const __m256i& W1v){
    __m256i lo0 = mullo_const_u32(r0, PH_M0);
    __m256i lo1 = mullo_const_u32(r2, PH_M1);
    __m256i hi0 = mulhi_const_u32(r0, PH_M0);
    __m256i hi1 = mulhi_const_u32(r2, PH_M1);
    __m256i t0  = xor32(xor32(hi1, r1), kk0);
    __m256i t1  = lo1;
    __m256i t2  = xor32(xor32(hi0, r3), kk1);
    __m256i t3  = lo0;
    kk0 = add32(kk0, W0v); kk1 = add32(kk1, W1v);

    lo0 = mullo_const_u32(t0, PH_M0);
    lo1 = mullo_const_u32(t2, PH_M1);
    hi0 = mulhi_const_u32(t0, PH_M0);
    hi1 = mulhi_const_u32(t2, PH_M1);
    r0  = xor32(xor32(hi1, t1), kk0);
    r1  = lo1;
    r2  = xor32(xor32(hi0, t3), kk1);
    r3  = lo0;
    kk0 = add32(kk0, W0v); kk1 = add32(kk1, W1v);
  }

  inline void rounds10(__m256i& o0,__m256i& o1,__m256i& o2,__m256i& o3){
    __m256i r0=c0, r1=c1, r2=c2, r3=c3, kk0=k0, kk1=k1;
    const __m256i W0v = _mm256_set1_epi32((int)PH_W0);
    const __m256i W1v = _mm256_set1_epi32((int)PH_W1);
    two_rounds(r0,r1,r2,r3,kk0,kk1,W0v,W1v);
    two_rounds(r0,r1,r2,r3,kk0,kk1,W0v,W1v);
    two_rounds(r0,r1,r2,r3,kk0,kk1,W0v,W1v);
    two_rounds(r0,r1,r2,r3,kk0,kk1,W0v,W1v);
    two_rounds(r0,r1,r2,r3,kk0,kk1,W0v,W1v);
    o0=r0; o1=r1; o2=r2; o3=r3;
  }

  inline void next_block(__m256i& o0,__m256i& o1,__m256i& o2,__m256i& o3){
    rounds10(o0,o1,o2,o3);
    bump_nocarry(1);
  }

  void generate_u64(uint64_t* out, size_t n) noexcept {
    size_t i=0;
    while (i + 32 <= n) {
      __m256i a0,a1,a2,a3; next_block(a0,a1,a2,a3);
      __m256i b0,b1,b2,b3; next_block(b0,b1,b2,b3);
      __m256i c0v,c1v,c2v,c3v; next_block(c0v,c1v,c2v,c3v);
      __m256i d0,d1,d2,d3; next_block(d0,d1,d2,d3);
      __m256i u0 = pack_u64(a0,a1), u1 = pack_u64(a2,a3);
      __m256i u2 = pack_u64(b0,b1), u3 = pack_u64(b2,b3);
      __m256i u4 = pack_u64(c0v,c1v), u5 = pack_u64(c2v,c3v);
      __m256i u6 = pack_u64(d0,d1), u7 = pack_u64(d2,d3);
      _mm256_storeu_si256((__m256i*)(out + i +  0), u0);
      _mm256_storeu_si256((__m256i*)(out + i +  4), u1);
      _mm256_storeu_si256((__m256i*)(out + i +  8), u2);
      _mm256_storeu_si256((__m256i*)(out + i + 12), u3);
      _mm256_storeu_si256((__m256i*)(out + i + 16), u4);
      _mm256_storeu_si256((__m256i*)(out + i + 20), u5);
      _mm256_storeu_si256((__m256i*)(out + i + 24), u6);
      _mm256_storeu_si256((__m256i*)(out + i + 28), u7);
      i += 32;
    }
    while (i < n) {
      __m256i o0,o1,o2,o3; next_block(o0,o1,o2,o3);
      __m256i u0 = pack_u64(o0,o1), u1 = pack_u64(o2,o3);
      alignas(32) uint64_t tmp[8];
      _mm256_store_si256((__m256i*)(tmp+0), u0);
      _mm256_store_si256((__m256i*)(tmp+4), u1);
      for (int k=0; k<8 && i<n; ++k,++i) out[i] = tmp[k];
    }
  }
};

} // namespace ua
