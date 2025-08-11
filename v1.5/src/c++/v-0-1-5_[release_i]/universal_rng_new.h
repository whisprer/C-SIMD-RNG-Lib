#ifndef UNIVERSAL_RNG_MODERN_H
#define UNIVERSAL_RNG_MODERN_H

#ifdef __GNUC__
#include <cpuid.h>
#endif

#include <cstdint>
#include <memory>
#include <string>
#include <iostream>
#include <random>

// Force enable SSE2 and AVX2 for testing
has_sse2_support = true;
has_avx2_support = true;

// Define common structs and functions for RNG implementations
universal_rng_t* universal_rng_new(uint64_t seed, int algorithm_type, int precision_mode) {
    std::cout << "Creating RNG...\n";
    
    // Create a new universal RNG
    universal_rng_t* rng = new universal_rng_t;
    
    // Initialize all fields
    rng->state = nullptr;
    rng->next_u64 = nullptr;
    rng->next_double = nullptr;
    rng->generate_batch = nullptr;
    rng->free_func = nullptr;
    rng->implementation_type = RNG_IMPL_SCALAR;
    rng->algorithm_type = static_cast<RNGAlgorithmType>(algorithm_type);
    rng->precision_mode = static_cast<RNGPrecisionMode>(precision_mode);
    
    // Detect CPU features
    bool has_avx512_support = detect_avx512_support();
    bool has_avx2_support = detect_avx2_support();
    bool has_sse2_support = detect_sse2_support();
    
    // Detect best available implementation
    void* state = nullptr;

    std::cout << "CPU feature detection:\n";
    std::cout << "  SSE2: " << (has_sse2_support ? "Yes" : "No") << "\n";
    std::cout << "  AVX2: " << (has_avx2_support ? "Yes" : "No") << "\n";
    std::cout << "  AVX512: " << (has_avx512_support ? "Yes" : "No") << "\n";
    
    // Try AVX512 first if available
    #ifdef USE_AVX512
    if (has_avx512_support) {
        std::cout << "Trying AVX512 implementation...\n";
        try {
            state = avx512_new(seed);
            if (state) {
                std::cout << "Using AVX512 implementation\n";
                rng->implementation_type = RNG_IMPL_AVX512;
                rng->next_u64 = avx512_next_u64;
                rng->next_double = avx512_next_double;
                rng->generate_batch = avx512_next_batch;
                rng->free_func = avx512_free;
            } else {
                std::cout << "AVX512 implementation created null state\n";
            }
        } catch (const std::exception& e) {
            std::cout << "Exception in AVX512 initialization: " << e.what() << "\n";
        } catch (...) {
            std::cout << "Unknown exception in AVX512 initialization\n";
        }
    } else {
        std::cout << "AVX512 not supported by CPU, skipping\n";
    }
    #else
    std::cout << "AVX512 not compiled in, skipping\n";
    #endif
    
    // Try AVX2 if it's available
    if (!state) {
        #ifdef USE_AVX2
        if (has_avx2_support) {
            std::cout << "Trying AVX2 implementation...\n";
            try {
                state = avx2_new(seed);
                if (state) {
                    std::cout << "Using AVX2 implementation\n";
                    rng->implementation_type = RNG_IMPL_AVX2;
                    rng->next_u64 = avx2_next_u64;
                    rng->next_double = avx2_next_double;
                    rng->generate_batch = avx2_next_batch;
                    rng->free_func = avx2_free;
                } else {
                    std::cout << "AVX2 implementation created null state\n";
                }
            } catch (const std::exception& e) {
                std::cout << "Exception in AVX2 initialization: " << e.what() << "\n";
            } catch (...) {
                std::cout << "Unknown exception in AVX2 initialization\n";
            }
        } else {
            std::cout << "AVX2 not supported by CPU, skipping\n";
        }
        #else
        std::cout << "AVX2 not compiled in, skipping\n";
        #endif
    }
    
    // If AVX2 failed or wasn't available, try SSE2
    if (!state) {
        #ifdef USE_SSE2
        if (has_sse2_support) {
            std::cout << "Trying SSE2 implementation...\n";
            try {
                state = sse2_new(seed);
                if (state) {
                    std::cout << "Using SSE2 implementation\n";
                    rng->implementation_type = RNG_IMPL_SSE2;
                    rng->next_u64 = sse2_next_u64;
                    rng->next_double = sse2_next_double;
                    rng->generate_batch = sse2_next_batch;
                    rng->free_func = sse2_free;
                } else {
                    std::cout << "SSE2 implementation created null state\n";
                }
            } catch (const std::exception& e) {
                std::cout << "Exception in SSE2 initialization: " << e.what() << "\n";
            } catch (...) {
                std::cout << "Unknown exception in SSE2 initialization\n";
            }
        } else {
            std::cout << "SSE2 not supported by CPU, skipping\n";
        }
        #else
        std::cout << "SSE2 not compiled in, skipping\n";
        #endif
    }
    
    // If all else fails, fall back to scalar
    if (!state) {
        std::cout << "Using scalar implementation\n";
        try {
            state = scalar_new(seed);
            if (state) {
                rng->implementation_type = RNG_IMPL_SCALAR;
                rng->next_u64 = scalar_next_u64;
                rng->next_double = scalar_next_double;
                rng->generate_batch = scalar_next_batch;
                rng->free_func = scalar_free;
            } else {
                std::cout << "ERROR: Scalar implementation created null state\n";
                delete rng;
                return nullptr;
            }
        } catch (const std::exception& e) {
            std::cout << "Exception in scalar initialization: " << e.what() << "\n";
            delete rng;
            return nullptr;
        } catch (...) {
            std::cout << "Unknown exception in scalar initialization\n";
            delete rng;
            return nullptr;
        }
    }
    
    // Set the state and return
    rng->state = state;
    std::cout << "RNG created successfully\n";
    return rng;
}

#endif // UNIVERSAL_RNG_MODERN_H