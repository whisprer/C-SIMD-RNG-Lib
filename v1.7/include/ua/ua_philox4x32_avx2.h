#pragma once
#include <cstdint>
#include <cstddef>
#include <immintrin.h>
#include "ua_platform.h"

namespace ua {

// Philox4x32-10 constants
static constexpr uint32_t PHILOX_M0 = 0xD2511F53u;
static constexpr uint32_t PHILOX_M1 = 0xCD9E8D57u;
static constexpr uint32_t PHILOX_W0 = 0x9E3779B9u;
static constexpr uint32_t PHILOX_W1 = 0xBB67AE85u;

struct Philox4x32AVX2 {
  __m256i c0, c1, c2, c3;   // counters (per lane)
  __m256i k0, k1;           // keys (broadcast)

  static UA_FORCE_INLINE __m256i add32(__m256i a, __m256i b){ return _mm256_add_epi32(a,b); }
  static UA_FORCE_INLINE __m256i xor32(__m256i a, __m256i b){ return _mm256_xor_si256(a,b); }

  // 32x32->hi32 using two mul_epu32 lanes (even/odd) and merge.
  static UA_FORCE_INLINE __m256i mulhi_const_u32(__m256i x, uint32_t m){
    const __m256i M = _mm256_set1_epi32((int)m);
    __m256i even_prod = _mm256_mul_epu32(x, M);               // even lanes: 0,2,4,6
    __m256i even_hi   = _mm256_srli_epi64(even_prod, 32);
    __m256i odd_x     = _mm256_srli_epi64(x, 32);             // shift odd to even
    __m256i odd_prod  = _mm256_mul_epu32(odd_x, M);
    __m256i odd_hi    = _mm256_srli_epi64(odd_prod, 32);
    __m256i even32    = _mm256_and_si256(even_hi, _mm256_set1_epi64x(0x00000000FFFFFFFFull));
    __m256i odd32     = _mm256_slli_epi64(odd_hi, 32);
    return _mm256_or_si256(even32, odd32);
  }
  static UA_FORCE_INLINE __m256i mullo_const_u32(__m256i x, uint32_t m){
    return _mm256_mullo_epi32(x, _mm256_set1_epi32((int)m));
  }

  // Pack (hi32<<32)|lo32 into u64 lanes
  static UA_FORCE_INLINE __m256i pack_u64(__m256i hi32, __m256i lo32){
    __m256i hi = _mm256_slli_epi64(hi32, 32);
    __m256i lo = _mm256_and_si256(lo32, _mm256_set1_epi64x(0x00000000FFFFFFFFull));
    return _mm256_or_si256(hi, lo);
  }

  // SplitMix64 for seeding
  static uint64_t sm64(uint64_t& x){
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
    c0 = lane;
    c1 = _mm256_setzero_si256();
    c2 = _mm256_setzero_si256();
    c3 = _mm256_setzero_si256();
  }

  // cheap bump: c0 += inc (no carry), ok for bench scales
  UA_FORCE_INLINE void bump_counter_nocarry(int inc){
    c0 = _mm256_add_epi32(c0, _mm256_set1_epi32(inc));
  }

  // Two rounds fused (updates r and k)
  static UA_FORCE_INLINE void two_rounds(__m256i& r0,__m256i& r1,__m256i& r2,__m256i& r3,
                                         __m256i& kk0,__m256i& kk1,
                                         const __m256i& W0v,const __m256i& W1v){
    // round 1
    __m256i lo0 = mullo_const_u32(r0, PHILOX_M0);
    __m256i lo1 = mullo_const_u32(r2, PHILOX_M1);
    __m256i hi0 = mulhi_const_u32(r0, PHILOX_M0);
    __m256i hi1 = mulhi_const_u32(r2, PHILOX_M1);
    __m256i t0  = xor32(xor32(hi1, r1), kk0);
    __m256i t1  = lo1;
    __m256i t2  = xor32(xor32(hi0, r3), kk1);
    __m256i t3  = lo0;
    kk0 = add32(kk0, W0v); kk1 = add32(kk1, W1v);

    // round 2
    lo0 = mullo_const_u32(t0, PHILOX_M0);
    lo1 = mullo_const_u32(t2, PHILOX_M1);
    hi0 = mulhi_const_u32(t0, PHILOX_M0);
    hi1 = mulhi_const_u32(t2, PHILOX_M1);
    r0  = xor32(xor32(hi1, t1), kk0);
    r1  = lo1;
    r2  = xor32(xor32(hi0, t3), kk1);
    r3  = lo0;
    kk0 = add32(kk0, W0v); kk1 = add32(kk1, W1v);
  }

  UA_FORCE_INLINE void rounds10(__m256i& o0,__m256i& o1,__m256i& o2,__m256i& o3){
    __m256i r0=c0, r1=c1, r2=c2, r3=c3;
    __m256i kk0=k0, kk1=k1;
    const __m256i W0v = _mm256_set1_epi32((int)PHILOX_W0);
    const __m256i W1v = _mm256_set1_epi32((int)PHILOX_W1);
    // 5×(2 rounds)
    two_rounds(r0,r1,r2,r3,kk0,kk1,W0v,W1v);
    two_rounds(r0,r1,r2,r3,kk0,kk1,W0v,W1v);
    two_rounds(r0,r1,r2,r3,kk0,kk1,W0v,W1v);
    two_rounds(r0,r1,r2,r3,kk0,kk1,W0v,W1v);
    two_rounds(r0,r1,r2,r3,kk0,kk1,W0v,W1v);
    o0=r0; o1=r1; o2=r2; o3=r3;
  }

  UA_FORCE_INLINE void next_block(__m256i& o0,__m256i& o1,__m256i& o2,__m256i& o3){
    rounds10(o0,o1,o2,o3);
    bump_counter_nocarry(1);
  }

  // Generate: write 32×u64 per iter (4 blocks)
  void generate_u64(uint64_t* out, size_t n) noexcept {
    size_t i=0;
    while (i + 32 <= n) {
      __m256i a0,a1,a2,a3; next_block(a0,a1,a2,a3);
      __m256i b0,b1,b2,b3; next_block(b0,b1,b2,b3);
      __m256i c0v,c1v,c2v,c3v; next_block(c0v,c1v,c2v,c3v);
      __m256i d0,d1,d2,d3; next_block(d0,d1,d2,d3);

      __m256i u0 = pack_u64(a0,a1);
      __m256i u1 = pack_u64(a2,a3);
      __m256i u2 = pack_u64(b0,b1);
      __m256i u3 = pack_u64(b2,b3);
      __m256i u4 = pack_u64(c0v,c1v);
      __m256i u5 = pack_u64(c2v,c3v);
      __m256i u6 = pack_u64(d0,d1);
      __m256i u7 = pack_u64(d2,d3);

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
    // tail
    while (i < n) {
      __m256i o0,o1,o2,o3; next_block(o0,o1,o2,o3);
      __m256i u0 = pack_u64(o0,o1);
      __m256i u1 = pack_u64(o2,o3);
      alignas(32) uint64_t tmp[8];
      _mm256_store_si256((__m256i*)(tmp+0), u0);
      _mm256_store_si256((__m256i*)(tmp+4), u1);
      for (int k=0; k<8 && i<n; ++k,++i) out[i] = tmp[k];
    }
  }

  void generate_double(double* out, size_t n) noexcept {
    constexpr double inv = 1.0 / double(1ull<<53);
    size_t i=0;
    while (i + 32 <= n) {
      __m256i a0,a1,a2,a3; next_block(a0,a1,a2,a3);
      __m256i b0,b1,b2,b3; next_block(b0,b1,b2,b3);
      __m256i c0v,c1v,c2v,c3v; next_block(c0v,c1v,c2v,c3v);
      __m256i d0,d1,d2,d3; next_block(d0,d1,d2,d3);

      __m256i u0 = pack_u64(a0,a1);
      __m256i u1 = pack_u64(a2,a3);
      __m256i u2 = pack_u64(b0,b1);
      __m256i u3 = pack_u64(b2,b3);
      __m256i u4 = pack_u64(c0v,c1v);
      __m256i u5 = pack_u64(c2v,c3v);
      __m256i u6 = pack_u64(d0,d1);
      __m256i u7 = pack_u64(d2,d3);

      alignas(32) uint64_t tmp[32];
      _mm256_store_si256((__m256i*)(tmp+ 0), u0);
      _mm256_store_si256((__m256i*)(tmp+ 4), u1);
      _mm256_store_si256((__m256i*)(tmp+ 8), u2);
      _mm256_store_si256((__m256i*)(tmp+12), u3);
      _mm256_store_si256((__m256i*)(tmp+16), u4);
      _mm256_store_si256((__m256i*)(tmp+20), u5);
      _mm256_store_si256((__m256i*)(tmp+24), u6);
      _mm256_store_si256((__m256i*)(tmp+28), u7);

      out[i+ 0] = double(tmp[ 0] >> 11) * inv;
      out[i+ 1] = double(tmp[ 1] >> 11) * inv;
      out[i+ 2] = double(tmp[ 2] >> 11) * inv;
      out[i+ 3] = double(tmp[ 3] >> 11) * inv;
      out[i+ 4] = double(tmp[ 4] >> 11) * inv;
      out[i+ 5] = double(tmp[ 5] >> 11) * inv;
      out[i+ 6] = double(tmp[ 6] >> 11) * inv;
      out[i+ 7] = double(tmp[ 7] >> 11) * inv;
      out[i+ 8] = double(tmp[ 8] >> 11) * inv;
      out[i+ 9] = double(tmp[ 9] >> 11) * inv;
      out[i+10] = double(tmp[10] >> 11) * inv;
      out[i+11] = double(tmp[11] >> 11) * inv;
      out[i+12] = double(tmp[12] >> 11) * inv;
      out[i+13] = double(tmp[13] >> 11) * inv;
      out[i+14] = double(tmp[14] >> 11) * inv;
      out[i+15] = double(tmp[15] >> 11) * inv;
      out[i+16] = double(tmp[16] >> 11) * inv;
      out[i+17] = double(tmp[17] >> 11) * inv;
      out[i+18] = double(tmp[18] >> 11) * inv;
      out[i+19] = double(tmp[19] >> 11) * inv;
      out[i+20] = double(tmp[20] >> 11) * inv;
      out[i+21] = double(tmp[21] >> 11) * inv;
      out[i+22] = double(tmp[22] >> 11) * inv;
      out[i+23] = double(tmp[23] >> 11) * inv;
      out[i+24] = double(tmp[24] >> 11) * inv;
      out[i+25] = double(tmp[25] >> 11) * inv;
      out[i+26] = double(tmp[26] >> 11) * inv;
      out[i+27] = double(tmp[27] >> 11) * inv;
      out[i+28] = double(tmp[28] >> 11) * inv;
      out[i+29] = double(tmp[29] >> 11) * inv;
      out[i+30] = double(tmp[30] >> 11) * inv;
      out[i+31] = double(tmp[31] >> 11) * inv;
      i += 32;
    }
    // tail
    while (i < n) {
      __m256i o0,o1,o2,o3; next_block(o0,o1,o2,o3);
      __m256i u0 = pack_u64(o0,o1);
      __m256i u1 = pack_u64(o2,o3);
      alignas(32) uint64_t tmp[8];
      _mm256_store_si256((__m256i*)(tmp+0), u0);
      _mm256_store_si256((__m256i*)(tmp+4), u1);
      for (int k=0; k<8 && i<n; ++k,++i) {
        out[i] = double(tmp[k] >> 11) * inv;
      }
    }
  }
};

} // namespace ua
