#include "rng_includes.h"
#include "universal_rng.h"

#ifdef USE_SSE2
#include <emmintrin.h>
#include <memory>
#include <array>
#include <algorithm>
#include <stdexcept>

// Modern C++ SSE2 RNG Implementation
class SSE2RNGState {
public:
    // Constructors and factory method
    explicit SSE2RNGState(uint64_t seed) : next_idx_(2) {
        initialize_state(seed);
    }

    // Static factory method
    static std::unique_ptr<SSE2RNGState> create(uint64_t seed) {
        return std::make_unique<SSE2RNGState>(seed);
    }

    // Next 64-bit random number generation
    uint64_t next_u64() {
        // If we've used all pre-generated numbers, generate a new batch
        if (next_idx_ >= 2) {
            generate_batch();
        }
        
        return results_[next_idx_++];
    }

    // Next double in [0,1) range
    double next_double() {
        return (next_u64() >> 11) * (1.0 / (1ULL << 53));
    }

    // Batch generation
    void next_batch(uint64_t* results, size_t count) {
        size_t generated = 0;
        
        while (generated < count) {
            // How many values can we get from current batch
            size_t available = 2 - next_idx_;
            size_t to_copy = std::min(available, count - generated);
            
            // Copy values from current batch
            std::copy_n(results_.data() + next_idx_, to_copy, results + generated);
            
            generated += to_copy;
            next_idx_ += to_copy;
            
            // If we need more values, generate a new batch
            if (next_idx_ >= 2 && generated < count) {
                generate_batch();
                next_idx_ = 0;
            }
        }
    }

private:
    // Helper function for SSE2 bit rotation
    static inline __m128i rotl_sse2(__m128i x, int k) {
        return _mm_or_si128(
            _mm_slli_epi64(x, k),
            _mm_srli_epi64(x, 64 - k)
        );
    }

    // Generate a new batch of random numbers
    void generate_batch() {
        // Calculate new state and results
        __m128i s0 = s0_;
        __m128i s1 = s1_;
        
        // Calculate result: rotl(s0 + s1, 17) + s0
        __m128i sum = _mm_add_epi64(s0, s1);
        
        // Since SSE2 doesn't have a direct 64-bit rotation, we need to handle it manually
        // we use the helper function rotl_sse2 defined above
        __m128i rotated_sum = rotl_sse2(sum, 17);
        __m128i result_vec = _mm_add_epi64(rotated_sum, s0);
        
        // Store results
        _mm_storeu_si128((__m128i*)results_.data(), result_vec);
        
        // Update state
        s1 = _mm_xor_si128(s1, s0);
        s0_ = _mm_xor_si128(
                rotl_sse2(s0, 49),
                _mm_xor_si128(
                    s1,
                    _mm_slli_epi64(s1, 21)
                )
            );
        s1_ = rotl_sse2(s1, 28);
        
        // Reset index
        next_idx_ = 0;
    }

    // Initialize state with different seeds
    void initialize_state(uint64_t seed) {
        // Initialize 2 parallel generators with different seeds
        std::array<uint64_t, 2> seeds = {seed, seed + 1};
        std::array<uint64_t, 2> s0_vals, s1_vals;
        
        for (int i = 0; i < 2; i++) {
            // Use SplitMix64 seeding for each stream
            uint64_t z = (seeds[i] + 0x9e3779b97f4a7c15ULL);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
            s0_vals[i] = z ^ (z >> 31);
            
            z = (s0_vals[i] + 0x9e3779b97f4a7c15ULL);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
            s1_vals[i] = z ^ (z >> 31);
        }
        
        // Load initial states into SSE2 registers
        s0_ = _mm_loadu_si128((__m128i*)s0_vals.data());
        s1_ = _mm_loadu_si128((__m128i*)s1_vals.data());
        
        // Pre-generate first batch of random numbers
        _mm_storeu_si128((__m128i*)results_.data(), 
            _mm_add_epi64(
                rotl_sse2(_mm_add_epi64(s0_, s1_), 17), 
                s0_
            )
        );
    }

    // State variables
    __m128i s0_;
    __m128i s1_;
    std::array<uint64_t, 2> results_;
    size_t next_idx_;
};

// C-compatible function implementations
extern "C" {
    void* sse2_new(uint64_t seed) {
        return SSE2RNGState::create(seed).release();
    }

    uint64_t sse2_next_u64(void* state) {
        return static_cast<SSE2RNGState*>(state)->next_u64();
    }

    double sse2_next_double(void* state) {
        return static_cast<SSE2RNGState*>(state)->next_double();
    }

    void sse2_next_batch(void* state, uint64_t* results, size_t count) {
        static_cast<SSE2RNGState*>(state)->next_batch(results, count);
    }

    void sse2_free(void* state) {
        delete static_cast<SSE2RNGState*>(state);
    }
}

#endif // USE_SSE2