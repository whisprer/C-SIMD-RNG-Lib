#ifndef AVX512_IMPL_H
#define AVX512_IMPL_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <array>
#include <string>

#ifdef _WIN32
#include <immintrin.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <x86intrin.h>
#endif

// Class declaration for AVX-512
class AVX512RNGState {
public:
    explicit AVX512RNGState(uint64_t seed);
    static std::unique_ptr<AVX512RNGState> create(uint64_t seed);
    uint64_t next_u64();
    double next_double();
    void next_batch(uint64_t* results, size_t count);
    const char* get_implementation_name() const;

private:
    void generate_batch();
    void initialize_state(uint64_t seed);
    static __m512i rotl_avx512(__m512i x, int k);

    __m512i s0_;
    __m512i s1_;
    std::array<uint64_t, 8> results_;
    size_t next_idx_;
};

// Function declarations for AVX-512 implementation
void* avx512_new(uint64_t seed);
uint64_t avx512_next_u64(void* state);
double avx512_next_double(void* state);
void avx512_next_batch(void* state, uint64_t* results, size_t count);
void avx512_free(void* state);

#endif // AVX512_IMPL_H