// src/philox4x32_scalar.cpp
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "rng_common.h"

namespace ua {

UA_FORCE_INLINE static void mulhilo32(std::uint32_t a, std::uint32_t b,
                                      std::uint32_t& hi, std::uint32_t& lo) noexcept {
  std::uint64_t p = static_cast<std::uint64_t>(a) * static_cast<std::uint64_t>(b);
  hi = static_cast<std::uint32_t>(p >> 32);
  lo = static_cast<std::uint32_t>(p);
}

struct Philox4x32_10_Scalar {
  // per-backend constants (scoped -> no TU collisions)
  static constexpr std::uint32_t M0 = 0xD2511F53u;
  static constexpr std::uint32_t M1 = 0xCD9E8D57u;
  static constexpr std::uint32_t W0 = 0x9E3779B9u;
  static constexpr std::uint32_t W1 = 0xBB67AE85u;

  std::uint32_t c0, c1, c2, c3;
  std::uint32_t k0, k1;

  explicit Philox4x32_10_Scalar(std::uint64_t seed, std::uint64_t stream) noexcept {
    SplitMix64 sm(seed);
    std::uint64_t k = sm.next();
    k0 = static_cast<std::uint32_t>(k);
    k1 = static_cast<std::uint32_t>(k >> 32);
    std::uint64_t s2 = sm.next();
    c0 = static_cast<std::uint32_t>(stream);
    c1 = static_cast<std::uint32_t>(stream >> 32);
    c2 = static_cast<std::uint32_t>(s2);
    c3 = static_cast<std::uint32_t>(s2 >> 32);
  }

  UA_FORCE_INLINE void bump_key() noexcept { k0 += W0; k1 += W1; }

  UA_FORCE_INLINE void round_once(std::uint32_t& x0, std::uint32_t& x1,
                                  std::uint32_t& x2, std::uint32_t& x3,
                                  std::uint32_t k0r, std::uint32_t k1r) noexcept {
    std::uint32_t hi0, lo0, hi1, lo1;
    mulhilo32(M0, x0, hi0, lo0);
    mulhilo32(M1, x2, hi1, lo1);

    std::uint32_t y0 = hi1 ^ x1 ^ k0r;
    std::uint32_t y1 = lo1;
    std::uint32_t y2 = hi0 ^ x3 ^ k1r;
    std::uint32_t y3 = lo0;

    x0 = y0; x1 = y1; x2 = y2; x3 = y3;
  }

  UA_FORCE_INLINE void block(std::uint32_t out[4]) noexcept {
    std::uint32_t x0 = c0, x1 = c1, x2 = c2, x3 = c3;
    std::uint32_t kk0 = k0, kk1 = k1;

    for (int i = 0; i < 10; ++i) {
      round_once(x0, x1, x2, x3, kk0, kk1);
      kk0 += W0; kk1 += W1;
    }

    out[0]=x0; out[1]=x1; out[2]=x2; out[3]=x3;

    if (++c0 == 0) { if (++c1 == 0) { if (++c2 == 0) { ++c3; } } }
  }

  UA_FORCE_INLINE void skip_ahead_blocks(std::uint64_t n) noexcept {
    std::uint64_t t = static_cast<std::uint64_t>(c0) + (n & 0xFFFFFFFFULL);
    c0 = static_cast<std::uint32_t>(t);
    std::uint32_t carry = static_cast<std::uint32_t>(t >> 32);
    std::uint64_t hi = (n >> 32);
    std::uint64_t t1 = static_cast<std::uint64_t>(c1) + (hi & 0xFFFFFFFFULL) + carry;
    c1 = static_cast<std::uint32_t>(t1); carry = static_cast<std::uint32_t>(t1 >> 32);
    std::uint64_t t2 = static_cast<std::uint64_t>(c2) + (hi >> 32) + carry;
    c2 = static_cast<std::uint32_t>(t2); carry = static_cast<std::uint32_t>(t2 >> 32);
    std::uint64_t t3 = static_cast<std::uint64_t>(c3) + carry;
    c3 = static_cast<std::uint32_t>(t3);
  }

  UA_FORCE_INLINE void fill_u64(std::uint64_t* dst, std::size_t n) noexcept {
    while (n >= 2) {
      std::uint32_t out[4];
      block(out);
      dst[0] = (static_cast<std::uint64_t>(out[1]) << 32) | out[0];
      dst[1] = (static_cast<std::uint64_t>(out[3]) << 32) | out[2];
      dst += 2; n -= 2;
    }
    if (n) {
      std::uint32_t out[4];
      block(out);
      dst[0] = (static_cast<std::uint64_t>(out[1]) << 32) | out[0];
    }
  }
};

} // namespace ua
