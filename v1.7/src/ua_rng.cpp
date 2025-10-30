#include "ua/ua_rng.h"
#include "ua/ua_cpuid.h"
#include "ua/ua_xoshiro256ss_avx2.h"
#include "ua/ua_xoshiro256ss_avx512.h"
// scalar backend: header-only in this project
#include "ua/ua_xoshiro256ss_scalar.h"

#include <cstdlib>
#include <cstring>

namespace ua {

using ua::detail::Xoshiro256ssAVX2;
using ua::detail::Xoshiro256ssAVX512;
using ua::detail::Xoshiro256ssScalar;

// ---------------------------
// Small helpers
// ---------------------------
static inline bool eq_ci(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = ca - 'A' + 'a';
        if (cb >= 'A' && cb <= 'Z') cb = cb - 'A' + 'a';
        if (ca != cb) return false;
        ++a; ++b;
    }
    return *a == 0 && *b == 0;
}

// ---------------------------
// Backend adaptors (Scalar)
// ---------------------------
struct ScalarState { Xoshiro256ssScalar prng; explicit ScalarState(std::uint64_t s): prng(s){} };

static void scalar_gen_u64(void* p, std::uint64_t* out, std::size_t n) noexcept {
    auto* s = static_cast<ScalarState*>(p);
    s->prng.generate_u64(out, n);
}
static void scalar_gen_double(void* p, double* out, std::size_t n) noexcept {
    auto* s = static_cast<ScalarState*>(p);
    s->prng.generate_double(out, n);
}
static void scalar_gen_normal(void* p, double* out, std::size_t n) noexcept {
    auto* s = static_cast<ScalarState*>(p);
    s->prng.generate_normal(out, n);
}
static void scalar_jump(void* p) noexcept { (void)p; /* optional */ }
static void scalar_destroy(void* p) noexcept { delete static_cast<ScalarState*>(p); }

// ---------------------------
// Backend adaptors (AVX2)
// ---------------------------
static void avx2_gen_u64(void* p, std::uint64_t* out, std::size_t n) noexcept {
    static_cast<Xoshiro256ssAVX2*>(p)->generate_u64(out, n);
}
static void avx2_gen_double(void* p, double* out, std::size_t n) noexcept {
    static_cast<Xoshiro256ssAVX2*>(p)->generate_double(out, n);
}
static void avx2_gen_normal(void* p, double* out, std::size_t n) noexcept {
    static_cast<Xoshiro256ssAVX2*>(p)->generate_normal(out, n);
}
static void avx2_jump(void* p) noexcept { (void)p; /* optional */ }
static void avx2_destroy(void* p) noexcept { delete static_cast<Xoshiro256ssAVX2*>(p); }

// ---------------------------
// Backend adaptors (AVX-512F)
// ---------------------------
static void avx512_gen_u64(void* p, std::uint64_t* out, std::size_t n) noexcept {
    static_cast<Xoshiro256ssAVX512*>(p)->generate_u64(out, n);
}
static void avx512_gen_double(void* p, double* out, std::size_t n) noexcept {
    static_cast<Xoshiro256ssAVX512*>(p)->generate_double(out, n);
}
static void avx512_gen_normal(void* p, double* out, std::size_t n) noexcept {
    static_cast<Xoshiro256ssAVX512*>(p)->generate_normal(out, n);
}
static void avx512_jump(void* p) noexcept { (void)p; /* optional */ }
static void avx512_destroy(void* p) noexcept { delete static_cast<Xoshiro256ssAVX512*>(p); }

// ---------------------------
// Rng: ctor / dtor / moves
// ---------------------------
Rng::Rng(std::uint64_t seed) {
    // UA_FORCE_BACKEND=scalar|avx2|avx512
    const char* env = std::getenv("UA_FORCE_BACKEND");
    CpuFeatures f = query_cpu_features();

    if ((env && eq_ci(env,"avx512")) || (!env && f.avx512f)) {
        static const Vtbl v{ &avx512_gen_u64, &avx512_gen_double, &avx512_gen_normal, &avx512_jump, &avx512_destroy };
        vt_ = &v; tier_ = SimdTier::AVX512F;
        state_ = new Xoshiro256ssAVX512(seed);
        return;
    }
    if ((env && eq_ci(env,"avx2")) || (!env && f.avx2)) {
        static const Vtbl v{ &avx2_gen_u64, &avx2_gen_double, &avx2_gen_normal, &avx2_jump, &avx2_destroy };
        vt_ = &v; tier_ = SimdTier::AVX2;
        state_ = new Xoshiro256ssAVX2(seed);
        return;
    }
    // Fallback: scalar
    {
        static const Vtbl v{ &scalar_gen_u64, &scalar_gen_double, &scalar_gen_normal, &scalar_jump, &scalar_destroy };
        vt_ = &v; tier_ = SimdTier::Scalar;
        state_ = new ScalarState(seed);
    }
}

Rng::~Rng() {
    if (vt_ && state_) vt_->destroy(state_);
    vt_ = nullptr; state_ = nullptr;
}

Rng::Rng(Rng&& o) noexcept {
    vt_ = o.vt_; o.vt_ = nullptr;
    state_ = o.state_; o.state_ = nullptr;
    tier_ = o.tier_;
}

Rng& Rng::operator=(Rng&& o) noexcept {
    if (this != &o) {
        if (vt_ && state_) vt_->destroy(state_);
        vt_ = o.vt_; o.vt_ = nullptr;
        state_ = o.state_; o.state_ = nullptr;
        tier_ = o.tier_;
    }
    return *this;
}

// ---------------------------
// Rng: forwards used by bench
// ---------------------------
void Rng::generate_u64(std::uint64_t* out, std::size_t n) noexcept { vt_->gen_u64(state_, out, n); }
void Rng::generate_double(double* out, std::size_t n) noexcept      { vt_->gen_double(state_, out, n); }
void Rng::generate_normal(double* out, std::size_t n) noexcept      { vt_->gen_normal(state_, out, n); }
void Rng::jump() noexcept                                           { vt_->jump(state_); }

} // namespace ua
