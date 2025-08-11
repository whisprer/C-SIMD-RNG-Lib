@echo off
setlocal enabledelayedexpansion

:: Set up default variables
set BUILD_TYPE=Release
set OUTPUT_NAME=universal_rng_bench
set BENCH_NAME=simple_benchmark
set ENHANCED_BENCH_NAME=enhanced_benchmark
set BITWIDTH_TEST_NAME=bit_width_test
set BITWIDTH_BENCH_NAME=bitwidth_benchmark
set ENHANCED_BITWIDTH_BENCH_NAME=enhanced_bitwidth_benchmark
set CXX=g++
set CXXFLAGS=-std=c++17 -O3 -Wall
set SOURCE_FILES=universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_bench.cpp universal_rng_scalar.cpp main_stub.cpp

:: Set SIMD flags for compilation
set MACRO_FLAGS=-UUSE_AVX512 -DUSE_AVX2 -DUSE_SSE2 -DUSE_AVX
set SIMD_FLAGS=-mavx -mavx2

echo Universal RNG Compilation
echo ------------------------
echo Compiler: %CXX%
echo Build Type: %BUILD_TYPE%
echo Output: %OUTPUT_NAME%.exe
echo AVX2, SSE2 (AVX512 disabled for stability)

:: Main compilation
if "!CXX!"=="g++" (
    echo Building with g++...
    g++ %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% %SOURCE_FILES% -o %OUTPUT_NAME%.exe
    
    echo Building basic benchmark...
    g++ %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_rng_scalar.cpp simple_benchmark.cpp -o %BENCH_NAME%.exe
    
    echo Building enhanced benchmark with comparison to STL...
    g++ %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_rng_scalar.cpp enhanced_benchmark.cpp -o %ENHANCED_BENCH_NAME%.exe
    
    echo Building bit width test...
    g++ %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_rng_scalar.cpp bit_width_test.cpp -o %BITWIDTH_TEST_NAME%.exe
    
    echo Building bit width benchmark...
    g++ %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_rng_scalar.cpp bitwidth_benchmark.cpp -o %BITWIDTH_BENCH_NAME%.exe
    
    echo Building enhanced bit width benchmark...
    g++ %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_rng_scalar.cpp enhanced_bitwidth_benchmark.cpp -o %ENHANCED_BITWIDTH_BENCH_NAME%.exe
) else (
    echo Building with MSVC...
    cl %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% %SOURCE_FILES% /Fe%OUTPUT_NAME%.exe
    
    echo Building basic benchmark...
    cl %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_rng_scalar.cpp simple_benchmark.cpp /Fe%BENCH_NAME%.exe
    
    echo Building enhanced benchmark with comparison to STL...
    cl %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_rng_scalar.cpp enhanced_benchmark.cpp /Fe%ENHANCED_BENCH_NAME%.exe
    
    echo Building bit width test...
    cl %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_rng_scalar.cpp bit_width_test.cpp /Fe%BITWIDTH_TEST_NAME%.exe
    
    echo Building bit width benchmark...
    cl %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_rng_scalar.cpp bitwidth_benchmark.cpp /Fe%BITWIDTH_BENCH_NAME%.exe
    
    echo Building enhanced bit width benchmark...
    cl %CXXFLAGS% %MACRO_FLAGS% %SIMD_FLAGS% universal_rng.cpp universal_rng_api.cpp cpu_detect.cpp universal_rng_bitwidth.cpp runtime_detect.cpp simd_avx2.cpp simd_sse2.cpp simd_avx512.cpp universal_rng_scalar.cpp enhanced_bitwidth_benchmark.cpp /Fe%ENHANCED_BITWIDTH_BENCH_NAME%.exe
)

if %ERRORLEVEL% neq 0 (
    echo Compilation failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo Build successful: %OUTPUT_NAME%.exe
echo Build successful: %BENCH_NAME%.exe
echo Build successful: %ENHANCED_BENCH_NAME%.exe
echo Build successful: %BITWIDTH_TEST_NAME%.exe
echo Build successful: %BITWIDTH_BENCH_NAME%.exe
echo Build successful: %ENHANCED_BITWIDTH_BENCH_NAME%.exe
echo.

:: Select which executable to run (main test or benchmark)
echo What would you like to run?
echo 1. Main test
echo 2. Basic benchmark
echo 3. Enhanced benchmark with STL comparison
echo 4. Bit width test
echo 5. Bit width benchmark
echo 6. Enhanced bit width benchmark (single + batch mode)
set /p run_choice="Enter your choice (1-6): "

if "%run_choice%"=="6" (
    echo Running enhanced bit width benchmark...
    %ENHANCED_BITWIDTH_BENCH_NAME%.exe
) else if "%run_choice%"=="5" (
    echo Running bit width benchmark...
    %BITWIDTH_BENCH_NAME%.exe
) else if "%run_choice%"=="4" (
    echo Running bit width test...
    %BITWIDTH_TEST_NAME%.exe
) else if "%run_choice%"=="3" (
    echo Running enhanced benchmark...
    %ENHANCED_BENCH_NAME%.exe
) else if "%run_choice%"=="2" (
    echo Running basic benchmark...
    %BENCH_NAME%.exe
) else (
    echo Running main test...
    %OUTPUT_NAME%.exe
)

exit /b 0
