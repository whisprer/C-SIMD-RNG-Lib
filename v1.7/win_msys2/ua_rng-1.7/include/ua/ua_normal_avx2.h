#pragma once
#include <cstddef>
#include <cmath>
#include <vector>
#include "ua_platform.h"

namespace ua {

// A simple Box–Muller transform that leans on the fast AVX2 uniform generator upstream.
// We batch-generate uniforms via rng.generate_double() and then transform to normals.
// This keeps the transcendentals scalar (libm) but still flies in bulk on your i7-8850H.
template <class RngT>
struct NormalGenerator {
  RngT& rng;
  explicit NormalGenerator(RngT& r) : rng(r) {}

  void generate(double* out, size_t n) {
    size_t i = 0;
    // process pairs
    const size_t chunk = 1 << 14; // 16k doubles buffer
    std::vector<double> tmp;
    tmp.resize(chunk);

    while (i < n) {
      size_t need = n - i;
      size_t grab = need < chunk ? need : chunk;
      // ensure even count for Box–Muller
      if (grab & 1) grab -= 1;
      if (grab == 0) grab = (need >= 2 ? 2 : 0);
      if (grab == 0) break;

      rng.generate_double(tmp.data(), grab);
      for (size_t k = 0; k < grab; k += 2) {
        double u1 = tmp[k];
        double u2 = tmp[k+1];
        if (u1 <= 0.0) u1 = std::ldexp(1.0, -53); // avoid log(0)
        double r = std::sqrt(-2.0 * std::log(u1));
        double t = 6.28318530717958647692 * u2;  // 2*pi
        double z0 = r * std::cos(t);
        double z1 = r * std::sin(t);
        out[i++] = z0;
        if (i < n) out[i++] = z1;
      }
    }

    // odd tail (generate two, use one)
    if (i < n) {
      double uv[2];
      rng.generate_double(uv, 2);
      double u1 = uv[0] <= 0.0 ? std::ldexp(1.0,-53) : uv[0];
      double u2 = uv[1];
      double r = std::sqrt(-2.0 * std::log(u1));
      double t = 6.28318530717958647692 * u2;
      out[i++] = r * std::cos(t);
    }
  }
};

} // namespace ua
