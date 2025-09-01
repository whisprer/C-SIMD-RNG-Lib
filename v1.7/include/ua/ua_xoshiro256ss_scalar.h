#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>

namespace ua::detail {

struct Xoshiro256ssScalar {
  std::uint64_t s0, s1, s2, s3;

  static constexpr std::uint64_t rotl(std::uint64_t x, int k) noexcept {
    return (x << k) | (x >> (64 - k));
  }

  static std::uint64_t splitmix64(std::uint64_t& x) noexcept {
    x += 0x9e3779b97f4a7c15ull;
    std::uint64_t z = x;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
  }

  explicit Xoshiro256ssScalar(std::uint64_t seed) noexcept {
    std::uint64_t x = seed;
    s0 = splitmix64(x); s1 = splitmix64(x);
    s2 = splitmix64(x); s3 = splitmix64(x);
  }

  std::uint64_t next_u64() noexcept {
    const std::uint64_t result = rotl(s1 * 5u, 7) * 9u;
    const std::uint64_t t = s1 << 17;
    s2 ^= s0; s3 ^= s1; s1 ^= s2; s0 ^= s3;
    s2 ^= t;
    s3 = rotl(s3, 45);
    return result;
  }

  void generate_u64(std::uint64_t* out, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; ++i) out[i] = next_u64();
  }

  void generate_double(double* out, std::size_t n) noexcept {
    constexpr double inv = 1.0 / double(1ull << 53);
    for (std::size_t i = 0; i < n; ++i) {
      std::uint64_t x = next_u64();
      out[i] = double(x >> 11) * inv; // [0,1)
    }
  }

  void generate_normal(double* out, std::size_t n) noexcept {
    std::size_t i = 0;
    while (i < n) {
      double u = 2.0 * next_uniform() - 1.0;
      double v = 2.0 * next_uniform() - 1.0;
      double s = u*u + v*v;
      if (s == 0.0 || s >= 1.0) continue;
      double k = std::sqrt(-2.0 * std::log(s) / s);
      out[i++] = u * k;
      if (i < n) out[i++] = v * k;
    }
  }

  void jump() noexcept {
    static constexpr std::uint64_t J[] = {
      0x180ec6d33cfd0abaULL, 0xd5a61266f0c9392cULL,
      0xa9582618e03fc9aaULL, 0x39abdc4529b1661cULL
    };
    std::uint64_t s0n=0, s1n=0, s2n=0, s3n=0;
    for (int i = 0; i < 4; ++i) {
      for (int b = 0; b < 64; ++b) {
        if (J[i] & (1ull << b)) { s0n ^= s0; s1n ^= s1; s2n ^= s2; s3n ^= s3; }
        (void)next_u64();
      }
    }
    s0 = s0n; s1 = s1n; s2 = s2n; s3 = s3n;
  }

private:
  inline double next_uniform() noexcept {
    constexpr double inv = 1.0 / double(1ull << 53);
    return double(next_u64() >> 11) * inv;
  }
};

} // namespace ua::detail

