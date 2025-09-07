cmake_minimum_required(VERSION 3.20)
project(ua_rng VERSION 1.7.0 LANGUAGES CXX)

# Options
option(UA_ENABLE_AVX2   "Build AVX2 backend TU" ON)
option(UA_ENABLE_AVX512 "Build AVX-512F backend TU" ON)
option(UA_STREAM_STORES "Use streaming stores when aligned" OFF)
option(UA_BUILD_BENCH   "Build benchmark app" ON)
option(UA_ENABLE_LTO    "Enable link-time optimization" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Global sane opts (portable; specific ISA flags added per-source below)
if (MSVC)
  add_compile_options(/O2 /DNOMINMAX /permissive- /Zc:__cplusplus)
else()
  add_compile_options(-O3 -fno-exceptions -fno-rtti -fno-omit-frame-pointer)
endif()

if (UA_ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT LTO_OK OUTPUT LTO_MSG)
  if (LTO_OK)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  endif()
endif()

add_library(ua_rng STATIC
  src/ua_cpuid.cpp
  src/ua_rng.cpp
  src/xoshiro256ss_scalar.cpp
)

target_include_directories(ua_rng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(ua_rng PUBLIC
  $<$<BOOL:${UA_STREAM_STORES}>:UA_STREAM_STORES=1>
)

# AVX2 backend TU compiled with AVX2 flags
if (UA_ENABLE_AVX2)
  target_sources(ua_rng PRIVATE src/xoshiro256ss_avx2.cpp)
  if (MSVC)
    set_source_files_properties(src/xoshiro256ss_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
  else()
    set_source_files_properties(src/xoshiro256ss_avx2.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mfma")
  endif()
  target_compile_definitions(ua_rng PRIVATE UA_BUILD_WITH_AVX2=1)
endif()

# AVX-512 backend TU compiled with AVX512 flags
if (UA_ENABLE_AVX512)
  target_sources(ua_rng PRIVATE src/xoshiro256ss_avx512.cpp)
  if (MSVC)
    # VS uses /arch:AVX512 from VS 19.29+ for some SKUs; fallback to /Qx:CORE-AVX512 for Intel compilers
    set_source_files_properties(src/xoshiro256ss_avx512.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX512")
  else()
    # Conservative set; widen if your target supports more
    set_source_files_properties(src/xoshiro256ss_avx512.cpp PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512dq -mavx512vl")
  endif()
  target_compile_definitions(ua_rng PRIVATE UA_BUILD_WITH_AVX512=1)
endif()

# Bench
if (UA_BUILD_BENCH)
  add_executable(ua_rng_bench bench/bench_main.cpp)
  target_link_libraries(ua_rng_bench PRIVATE ua_rng)
  if (MSVC)
    target_compile_options(ua_rng_bench PRIVATE /arch:AVX2)
  else()
    target_compile_options(ua_rng_bench PRIVATE -march=native -mtune=native)
  endif()
endif()
