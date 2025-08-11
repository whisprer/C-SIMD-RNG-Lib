#include "universal_rng.h"
#include "advanced_benchmarking.h"
#include <iostream>
#include <vector>
#include <memory>
#include <iomanip>
#include <algorithm>

// Configuration for multi-algorithm benchmark
class MultiAlgorithmBenchmark {
public:
    // Configuration parameters
    static constexpr size_t NUM_ITERATIONS = 100000000;
    static constexpr int NUM_RUNS = 5;

    // Benchmark algorithms and precision modes
    const std::vector<RNGAlgorithmType> algorithms = {
        RNG_ALGORITHM_XOROSHIRO,
        RNG_ALGORITHM_WYRAND
    };

    const std::vector<RNGPrecisionMode> precision_modes = {
        RNG_PRECISION_DOUBLE,
        RNG_PRECISION_SINGLE
    };

    // Run comprehensive multi-algorithm benchmark
    void run() {
        // Store benchmark results for potential comparison
        std::vector<std::unique_ptr<BenchmarkResult>> all_results;

        // Iterate through algorithms and precision modes
        for (auto algorithm : algorithms) {
            for (auto precision : precision_modes) {
                std::cout << "\n=== Benchmarking Algorithm " 
                          << static_cast<int>(algorithm) 
                          << ", Precision " 
                          << static_cast<int>(precision) 
                          << " ===\n";
                
                // Store results for this configuration
                std::vector<std::unique_ptr<BenchmarkResult>> config_results;
                
                // Multiple runs for statistical significance
                std::unique_ptr<BenchmarkResult> previous_result;
                
                for (int run = 0; run < NUM_RUNS; ++run) {
                    std::cout << "Run " << (run + 1) << ": ";
                    
                    // Create benchmark result
                    auto current_result = BenchmarkResult::create(
                        NUM_ITERATIONS,
                        static_cast<int>(algorithm),
                        static_cast<int>(precision)
                    );
                    
                    // Run benchmark
                    if (!current_result->run(previous_result.get())) {
                        std::cerr << "Benchmark failed\n";
                        continue;
                    }
                    
                    // Print results
                    std::cout << "Total Time: " 
                              << current_result->get_total_time_ns() << " ns, "
                              << "Rate: " << std::fixed << std::setprecision(2)
                              << current_result->get_generation_rate() << " numbers/sec, "
                              << "Performance Delta: " 
                              << current_result->get_performance_delta() << "%\n";
                    
                    // Export to CSV
                    std::string filename = generate_csv_filename(
                        algorithm, precision, run + 1
                    );
                    current_result->export_to_csv(filename);
                    
                    // Store results
                    config_results.push_back(std::move(current_result));
                    previous_result = config_results.back().get();
                }
                
                // Add to overall results
                all_results.insert(
                    all_results.end(), 
                    std::make_move_iterator(config_results.begin()),
                    std::make_move_iterator(config_results.end())
                );
            }
        }
    }

private:
    // Generate CSV filename with algorithm, precision, and run details
    std::string generate_csv_filename(
        RNGAlgorithmType algorithm, 
        RNGPrecisionMode precision, 
        int run
    ) const {
        std::ostringstream filename;
        filename << "benchmark_algo" 
                 << static_cast<int>(algorithm) 
                 << "_prec" 
                 << static_cast<int>(precision) 
                 << "_run" 
                 << run 
                 << ".csv";
        return filename.str();
    }
};

int main() {
    try {
        MultiAlgorithmBenchmark benchmark;
        benchmark.run();
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
