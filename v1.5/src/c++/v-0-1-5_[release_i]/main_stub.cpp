#include "universal_rng.h"
#include <iostream>
#include <vector>
#include <iomanip>

int main() {
    std::cout << "Universal RNG Test\n";
    std::cout << "================\n\n";
    
    try {
        // Create RNG
        std::cout << "Creating Xoroshiro RNG...\n";
        uint64_t seed = 42;
        universal_rng_t* rng = universal_rng_new(seed, 0, 1); // Xoroshiro, double precision
        
        if (!rng) {
            std::cerr << "Failed to create RNG!\n";
            return 1;
        }
        
        std::cout << "RNG created successfully!\n";
        
        // Show implementation information
        const char* impl = universal_rng_get_implementation(rng);
        std::cout << "Using implementation: " << impl << "\n\n";
        
        // Generate some random numbers
        std::cout << "Generating 5 random values:\n";
        for (int i = 0; i < 5; i++) {
            uint64_t value = universal_rng_next_u64(rng);
            std::cout << "  Value " << i+1 << ": " << value << "\n";
        }
        
        // Generate some random doubles
        std::cout << "\nGenerating 3 random doubles:\n";
        for (int i = 0; i < 3; i++) {
            double value = universal_rng_next_double(rng);
            std::cout << "  Value " << i+1 << ": " << std::setprecision(10) << value << "\n";
        }
        
        // Test batch generation
        std::cout << "\nTesting batch generation (10 values):\n";
        std::vector<uint64_t> batch(10);
        universal_rng_generate_batch(rng, batch.data(), batch.size());
        
        for (size_t i = 0; i < batch.size(); i++) {
            if (i < 5) {  // Show just the first 5 to keep output reasonable
                std::cout << "  Batch value " << i+1 << ": " << batch[i] << "\n";
            }
        }
        
        // Clean up
        std::cout << "\nCleaning up...\n";
        universal_rng_free_string(impl);
        universal_rng_free(rng);
        std::cout << "Cleanup successful!\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown exception caught!\n";
        return 1;
    }
    
    std::cout << "\nTest completed successfully!\n";
    return 0;
}