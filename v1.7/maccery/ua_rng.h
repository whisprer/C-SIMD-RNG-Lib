#pragma once
#include <cstddef>
#include <cstdint>

namespace ua {

// SIMD tier the dispatcher selected at runtime
enum class SimdTier : unsigned char {
    Scalar   = 0,
    AVX2     = 1,
    AVX512F  = 2,
};

class Rng {
public:
    explicit Rng(std::uint64_t seed = 0);
    ~Rng();
    Rng(Rng&&) noexcept;
    Rng& operator=(Rng&&) noexcept;

    Rng(const Rng&) = delete;
    Rng& operator=(const Rng&) = delete;

    // Low-level API used by the bench
    void generate_u64(std::uint64_t* out, std::size_t n) noexcept;
    void generate_double(double* out, std::size_t n) noexcept;   // [0,1)
    void generate_normal(double* out, std::size_t n) noexcept;   // N(0,1)
    void jump() noexcept;

    // Convenience wrappers (symmetric public API)
    inline void u64(std::uint64_t* out, std::size_t n) noexcept { generate_u64(out, n); }
    inline void uniform(double* out, std::size_t n) noexcept { generate_double(out, n); }
    inline void normal(double* out, std::size_t n) noexcept { generate_normal(out, n); }

    inline SimdTier simd_tier() const noexcept { return tier_; }

private:
    void* state_{nullptr};

    struct Vtbl {
        void (*gen_u64)(void*, std::uint64_t*, std::size_t) noexcept;
        void (*gen_double)(void*, double*, std::size_t) noexcept;
        void (*gen_normal)(void*, double*, std::size_t) noexcept;
        void (*jump)(void*) noexcept;
        void (*destroy)(void*) noexcept;
    };

    const Vtbl* vt_{nullptr};
    SimdTier tier_{SimdTier::Scalar};
};

} // namespace ua
