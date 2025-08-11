// src/universal_rng.cpp
#include "universal_rng.h"
#include "rng_common.h"
#include "rng_distributions.h"
#include "runtime_detect.h"
#include "simd_normal.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>
#include <memory>
#include <algorithm>

// --------- Pull backend class definitions into this TU (MSVC needs complete types) ----------
#include "xoshiro256ss_scalar.cpp"
#if defined(UA_ENABLE_AVX2)
  #include "simd_avx2_xoshiro256ss.cpp"
  #include "simd_avx2_philox4x32.cpp"
#endif
#if defined(UA_ENABLE_AVX512)
  #include "simd_avx512_xoshiro256ss.cpp"
  #include "simd_avx512_philox4x32.cpp"
#endif
#include "philox4x32_scalar.cpp"
#if defined(__aarch64__) || defined(__ARM_NEON)
  #include "simd_neon_philox4x32.cpp"
#endif
// -------------------------------------------------------------------------------------------

namespace ua {

#if defined(UA_ENABLE_AVX2)
using Xoshiro256ssAVX2_t   = Xoshiro256ssAVX2;
#endif
#if defined(UA_ENABLE_AVX512)
using Xoshiro256ssAVX512_t = Xoshiro256ssAVX512;
#endif
using Xoshiro256ssScalar_t = Xoshiro256ssScalar;

using PhiloxScalar_t  = Philox4x32_10_Scalar;
#if defined(UA_ENABLE_AVX2)
using PhiloxAVX2_t    = Philox4x32_10_AVX2;
#endif
#if defined(UA_ENABLE_AVX512)
using PhiloxAVX512_t  = Philox4x32_10_AVX512;
#endif

// ------------------------------- GeneratorContext ---------------------------------

class GeneratorContext {
public:
  explicit GeneratorContext(const Init& init)
  : algo_(init.algo),
    seed_(init.seed),
    stream_(init.stream),
    simd_(detect_simd_tier()),
    buf_cap_(init.buffer.capacity_u64 ? init.buffer.capacity_u64 : std::size_t(8192)),
    buf_(nullptr), buf_size_(0), buf_pos_(0)
  {
    // Optional env override for testing: UA_FORCE_SIMD=avx2|avx512|neon|scalar
#if defined(_WIN32)
    char* force = nullptr; size_t len = 0;
    if (_dupenv_s(&force, &len, "UA_FORCE_SIMD") == 0 && force && len) {
      std::unique_ptr<char, void(*)(void*)> g(force, [](void* p){ std::free(p); });
      if (_stricmp(force, "avx512") == 0) simd_ = SimdTier::AVX512;
      else if (_stricmp(force, "avx2") == 0) simd_ = SimdTier::AVX2;
      else if (_stricmp(force, "neon") == 0) simd_ = SimdTier::NEON;
      else if (_stricmp(force, "scalar") == 0) simd_ = SimdTier::Scalar;
    }
#else
    if (const char* force = std::getenv("UA_FORCE_SIMD")) {
      auto ieq = [](char a, char b){ return std::tolower(a)==std::tolower(b); };
      if (std::strlen(force)==6 && ieq(force[0],'a')&&ieq(force[1],'v')&&ieq(force[2],'x')&&ieq(force[3],'5')&&ieq(force[4],'1')&&ieq(force[5],'2'))
        simd_ = SimdTier::AVX512;
      else if (std::strlen(force)==4 && ieq(force[0],'a')&&ieq(force[1],'v')&&ieq(force[2],'x')&&ieq(force[3],'2'))
        simd_ = SimdTier::AVX2;
      else if (!strcasecmp(force,"neon"))   simd_ = SimdTier::NEON;
      else if (!strcasecmp(force,"scalar")) simd_ = SimdTier::Scalar;
    }
#endif

    buf_ = static_cast<std::uint64_t*>(::operator new[](buf_cap_ * sizeof(std::uint64_t), std::align_val_t(64)));

    construct_backend();
    refill_buffer();
  }

  ~GeneratorContext() {
    destroy_backend();
    if (buf_) { ::operator delete[](buf_, std::align_val_t(64)); buf_ = nullptr; }
  }

  SimdTier  simd() const noexcept { return simd_; }
  Algorithm algo() const noexcept { return algo_; }

  // produce u64s
  void generate_u64(std::uint64_t* out, std::size_t n) {
    while (n) {
      if (buf_pos_ == buf_size_) refill_buffer();
      std::size_t take = (std::min)(n, buf_size_ - buf_pos_);
      std::memcpy(out, buf_ + buf_pos_, take * sizeof(std::uint64_t));
      out += take; n -= take; buf_pos_ += take;
    }
  }

  // produce doubles in [0,1)
  void generate_double(double* out, std::size_t n) {
    const std::size_t chunk = 4096;
    std::uint64_t tmp[chunk];
    while (n) {
      std::size_t take = (std::min)(n, chunk);
      generate_u64(tmp, take);
      for (std::size_t i=0;i<take;++i) out[i] = u64_to_unit_double(tmp[i]);
      out += take; n -= take;
    }
  }

  // produce N( mean, stddev )
  void generate_normal(double mean, double stddev, double* out, std::size_t n) {
    if (n == 0) return;

#if defined(UA_ENABLE_AVX512)
    if (simd_ == SimdTier::AVX512) {
      const std::size_t lanes = 8;
      std::uint64_t u[lanes], v[lanes];
      std::size_t i = 0;
      for (; i + lanes <= n; i += lanes) {
        generate_u64(u, lanes);
        generate_u64(v, lanes);
        simd_normal_avx512(u, v, lanes, mean, stddev, out + i);
      }
      for (; i < n; ++i) {
        std::uint64_t uu, vv;
        generate_u64(&uu, 1);
        generate_u64(&vv, 1);
        double u01 = u64_to_unit_double(uu);
        double v01 = u64_to_unit_double(vv);
        out[i] = mean + stddev * ZigguratNormal::sample(uu, u01, v01);
      }
      return;
    }
#endif
#if defined(UA_ENABLE_AVX2)
    if (simd_ == SimdTier::AVX2) {
      const std::size_t lanes = 4;
      std::uint64_t u[lanes], v[lanes];
      std::size_t i = 0;
      for (; i + lanes <= n; i += lanes) {
        generate_u64(u, lanes);
        generate_u64(v, lanes);
        simd_normal_avx2(u, v, lanes, mean, stddev, out + i);
      }
      for (; i < n; ++i) {
        std::uint64_t uu, vv;
        generate_u64(&uu, 1);
        generate_u64(&vv, 1);
        double u01 = u64_to_unit_double(uu);
        double v01 = u64_to_unit_double(vv);
        out[i] = mean + stddev * ZigguratNormal::sample(uu, u01, v01);
      }
      return;
    }
#endif
#if defined(__aarch64__) || defined(__ARM_NEON)
    if (simd_ == SimdTier::NEON) {
      const std::size_t lanes = 4;
      std::uint64_t u[lanes], v[lanes];
      std::size_t i = 0;
      for (; i + lanes <= n; i += lanes) {
        generate_u64(u, lanes);
        generate_u64(v, lanes);
        simd_normal_neon(u, v, lanes, mean, stddev, out + i);
      }
      for (; i < n; ++i) {
        std::uint64_t uu, vv;
        generate_u64(&uu, 1);
        generate_u64(&vv, 1);
        double u01 = u64_to_unit_double(uu);
        double v01 = u64_to_unit_double(vv);
        out[i] = mean + stddev * ZigguratNormal::sample(uu, u01, v01);
      }
      return;
    }
#endif
    // scalar fallback
    for (std::size_t i=0; i<n; ++i) {
      std::uint64_t uu, vv;
      generate_u64(&uu, 1);
      generate_u64(&vv, 1);
      double u01 = u64_to_unit_double(uu);
      double v01 = u64_to_unit_double(vv);
      out[i] = mean + stddev * ZigguratNormal::sample(uu, u01, v01);
    }
  }

private:
  using fill_fn_t = void(*)(void* self, std::uint64_t* dst, std::size_t n);

  template <typename T>
  static void fill_trampoline(void* self, std::uint64_t* dst, std::size_t n) {
    static_cast<T*>(self)->fill_u64(dst, n);
  }

  void construct_backend() {
    switch (algo_) {
      case Algorithm::Xoshiro256ss:
      {
#if defined(UA_ENABLE_AVX512)
        if (simd_ == SimdTier::AVX512) {
          backend_object_ = ::operator new(sizeof(Xoshiro256ssAVX512_t), std::align_val_t(64));
          new (backend_object_) Xoshiro256ssAVX512_t(seed_, stream_);
          fill_ = &fill_trampoline<Xoshiro256ssAVX512_t>;
          return;
        }
#endif
#if defined(UA_ENABLE_AVX2)
        if (simd_ == SimdTier::AVX2) {
          backend_object_ = ::operator new(sizeof(Xoshiro256ssAVX2_t), std::align_val_t(64));
          new (backend_object_) Xoshiro256ssAVX2_t(seed_, stream_);
          fill_ = &fill_trampoline<Xoshiro256ssAVX2_t>;
          return;
        }
#endif
        backend_object_ = ::operator new(sizeof(Xoshiro256ssScalar_t), std::align_val_t(64));
        new (backend_object_) Xoshiro256ssScalar_t(seed_, stream_);
        fill_ = &fill_trampoline<Xoshiro256ssScalar_t>;
        return;
      }

      case Algorithm::Philox4x32_10:
      {
#if defined(UA_ENABLE_AVX512)
        if (simd_ == SimdTier::AVX512) {
          backend_object_ = ::operator new(sizeof(PhiloxAVX512_t), std::align_val_t(64));
          new (backend_object_) PhiloxAVX512_t(seed_, stream_);
          fill_ = &fill_trampoline<PhiloxAVX512_t>;
          return;
        }
#endif
#if defined(UA_ENABLE_AVX2)
        if (simd_ == SimdTier::AVX2) {
          backend_object_ = ::operator new(sizeof(PhiloxAVX2_t), std::align_val_t(64));
          new (backend_object_) PhiloxAVX2_t(seed_, stream_);
          fill_ = &fill_trampoline<PhiloxAVX2_t>;
          return;
        }
#endif
        backend_object_ = ::operator new(sizeof(PhiloxScalar_t), std::align_val_t(64));
        new (backend_object_) PhiloxScalar_t(seed_, stream_);
        fill_ = &fill_trampoline<PhiloxScalar_t>;
        return;
      }
    }
  }

  void destroy_backend() noexcept {
    if (!backend_object_) return;
    switch (algo_) {
      case Algorithm::Xoshiro256ss:
      {
#if defined(UA_ENABLE_AVX512)
        if (simd_ == SimdTier::AVX512) { static_cast<Xoshiro256ssAVX512_t*>(backend_object_)->~Xoshiro256ssAVX512_t(); break; }
#endif
#if defined(UA_ENABLE_AVX2)
        if (simd_ == SimdTier::AVX2)   { static_cast<Xoshiro256ssAVX2_t*>(backend_object_)->~Xoshiro256ssAVX2_t();     break; }
#endif
        static_cast<Xoshiro256ssScalar_t*>(backend_object_)->~Xoshiro256ssScalar_t();
        break;
      }
      case Algorithm::Philox4x32_10:
      {
#if defined(UA_ENABLE_AVX512)
        if (simd_ == SimdTier::AVX512) { static_cast<PhiloxAVX512_t*>(backend_object_)->~PhiloxAVX512_t(); break; }
#endif
#if defined(UA_ENABLE_AVX2)
        if (simd_ == SimdTier::AVX2)   { static_cast<PhiloxAVX2_t*>(backend_object_)->~PhiloxAVX2_t();     break; }
#endif
        static_cast<PhiloxScalar_t*>(backend_object_)->~PhiloxScalar_t();
        break;
      }
    }
    ::operator delete(backend_object_, std::align_val_t(64));
    backend_object_ = nullptr;
  }

  void refill_buffer() {
    fill_(backend_object_, buf_, buf_cap_);
    buf_size_ = buf_cap_;
    buf_pos_  = 0;
  }

private:
  Algorithm      algo_;
  std::uint64_t  seed_;
  std::uint64_t  stream_;
  SimdTier       simd_;

  std::size_t    buf_cap_;
  std::uint64_t* buf_;
  std::size_t    buf_size_;
  std::size_t    buf_pos_;

  void*          backend_object_ = nullptr;
  fill_fn_t      fill_ = nullptr;
};

// ------------------------------- UniversalRng API --------------------------------

UniversalRng::UniversalRng(const Init& init)
  : ctx_(std::make_unique<GeneratorContext>(init)) {}

UniversalRng::~UniversalRng() = default;

void UniversalRng::generate_u64(std::uint64_t* out, std::size_t n) {
  ctx_->generate_u64(out, n);
}

void UniversalRng::generate_double(double* out, std::size_t n) {
  ctx_->generate_double(out, n);
}

void UniversalRng::generate_normal(double mean, double stddev, double* out, std::size_t n) {
  ctx_->generate_normal(mean, stddev, out, n);
}

SimdTier UniversalRng::simd_tier() const noexcept { return ctx_->simd(); }

} // namespace ua
