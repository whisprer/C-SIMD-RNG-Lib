#pragma once
#include <cstddef>
#include <cmath>
#include <algorithm>

namespace ua {

// Marsaglia polar method (Box-Muller without trig).
// Uses the upstream RNG's generate_double() for uniforms.
//
// Algorithm:
//  - draw u,v ~ Unif(-1,1)
//  - s = u^2 + v^2; if s==0 or s>=1, reject and redraw
//  - factor = sqrt(-2 ln s / s)
//  - z0 = u*factor, z1 = v*factor
//
// We batch uniforms to amortize RNG calls.
template<class URNG>
struct PolarNormal {
  URNG& rng;
  explicit PolarNormal(URNG& r) : rng(r) {}

  void generate(double* out, size_t n) {
    size_t i = 0;
    // batch size: draw 2K uniforms at a time (u and v), then transform
    const size_t B = 1u << 14; // 16384
    std::unique_ptr<double[]> buf(new double[2*B]);

    while (i < n) {
      // number of normals still needed (rounded up to even since we produce pairs)
      size_t need = n - i;
      size_t target_pairs = (need + 1) / 2;
      size_t gen_pairs = std::min(target_pairs, B);

      // fill 2*gen_pairs uniforms in [0,1), map to (-1,1)
      double* U = buf.get();
      rng.generate_double(U, 2*gen_pairs);
      for (size_t k = 0; k < 2*gen_pairs; ++k) U[k] = U[k]*2.0 - 1.0;

      // transform with rejection
      size_t produced = 0;
      for (size_t p = 0; p < gen_pairs && i < n; ++p) {
        double u = U[2*p+0];
        double v = U[2*p+1];
        double s = u*u + v*v;
        if (s == 0.0 || s >= 1.0) continue; // reject
        double factor = std::sqrt(-2.0 * std::log(s) / s);
        out[i++] = u * factor;
        if (i < n) out[i++] = v * factor;
        ++produced;
      }

      // If we didn’t produce enough (too many rejections), loop again.
      // This is rare; the acceptance rate is ~78.5%.
      if (produced == 0) {
        // ensure progress: generate a tiny emergency pair
        double uvr[2];
        rng.generate_double(uvr, 2);
        double u = uvr[0]*2.0 - 1.0;
        double v = uvr[1]*2.0 - 1.0;
        double s = u*u + v*v;
        if (s > 0.0 && s < 1.0) {
          double factor = std::sqrt(-2.0 * std::log(s) / s);
          out[i++] = u * factor;
          if (i < n) out[i++] = v * factor;
        }
      }
    }
  }
};

} // namespace ua
