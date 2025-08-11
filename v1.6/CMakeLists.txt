cmake_minimum_required(VERSION 3.20)

project(UniversalRng
  VERSION 1.6.0
  LANGUAGES CXX)

# ---------------- SIMD options ----------------
option(ENABLE_AVX2 "Build AVX2 kernels" ON)
option(ENABLE_AVX512 "Build AVX-512 kernels" ON)
option(ENABLE_NEON "Build NEON kernels" OFF)

# Build shared and/or static
option(UA_BUILD_SHARED "Build UniversalRng as a shared library" ON)
option(UA_BUILD_STATIC "Build UniversalRng as a static library" ON)

# Benchmarks (already in your tree)
option(ENABLE_BENCH "Build benchmarks" ON)

# -------- compiler features/flags ------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (ENABLE_AVX2)
  add_compile_definitions(UA_ENABLE_AVX2=1)
endif()
if (ENABLE_AVX512)
  add_compile_definitions(UA_ENABLE_AVX512=1)
endif()

if (MSVC)
  if (ENABLE_AVX2)
    add_compile_options(/arch:AVX2)
  endif()
  if (ENABLE_AVX512)
    add_compile_options(/arch:AVX512)
  endif()
  add_compile_options(/Oi /Ot)
else()
  if (ENABLE_AVX2)
    add_compile_options(-mavx2 -mfma)
  endif()
  if (ENABLE_AVX512)
    add_compile_options(-mavx512f -mavx512dq -mfma)
  endif()
  if (ENABLE_NEON)
    add_compile_options(-mfpu=neon)
  endif()
endif()

# ------------- sources ----------------
set(UA_PUBLIC_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ua_export.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/universal_rng.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/runtime_detect.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/rng_distributions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/simd_normal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/simd_exp.h
)

set(UA_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/universal_rng.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime_detect.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/rng_distributions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/simd_normal_avx2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/simd_normal_avx512.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/simd_normal_neon.cpp
  # scalar SIMD helpers get included from the TU; keep separate files too
  ${CMAKE_CURRENT_SOURCE_DIR}/src/xoshiro256ss_scalar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/philox4x32_scalar.cpp
  # the avx/neon philox/xoshiro .cpps are #included by universal_rng.cpp (by design)
)

# ------------- targets ----------------
function(make_universal_rng libtype)
  if (${libtype} STREQUAL "SHARED")
    set(tgt universal_rng_shared)
    add_library(${tgt} SHARED ${UA_SOURCES})
    target_compile_definitions(${tgt} PRIVATE UA_BUILD_SHARED=1)
    set_target_properties(${tgt} PROPERTIES OUTPUT_NAME universal_rng)
  else()
    set(tgt universal_rng_static)
    add_library(${tgt} STATIC ${UA_SOURCES})
    set_target_properties(${tgt} PROPERTIES OUTPUT_NAME universal_rng)
  endif()

  target_include_directories(${tgt}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>)

  target_compile_features(${tgt} PUBLIC cxx_std_20)

  # Windows DLL import/export usage hint for dependents of the shared lib
  if (${libtype} STREQUAL "SHARED")
    target_compile_definitions(${tgt}
      INTERFACE UA_USE_SHARED=1)
  endif()

  # Versioning for shared libs on *nix
  set_target_properties(${tgt} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

  # install rules per target
  install(TARGETS ${tgt}
    EXPORT UniversalRngTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include)

  # headers installed once (guard by a var)
  set(_ua_headers_installed OFF CACHE INTERNAL "")
  if (NOT _ua_headers_installed)
    install(FILES ${UA_PUBLIC_HEADERS} DESTINATION include)
    set(_ua_headers_installed ON CACHE INTERNAL "")
  endif()
endfunction()

if (UA_BUILD_SHARED)
  make_universal_rng("SHARED")
endif()
if (UA_BUILD_STATIC)
  make_universal_rng("STATIC")
endif()

# ----------------- package config -----------------
include(CMakePackageConfigHelpers)

set(UA_INSTALL_CMAKEDIR "lib/cmake/UniversalRng" CACHE STRING "Install path for UniversalRng cmake files")

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/UniversalRngConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/UniversalRngConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/UniversalRngConfig.cmake"
  INSTALL_DESTINATION "${UA_INSTALL_CMAKEDIR}")

install(EXPORT UniversalRngTargets
  FILE UniversalRngTargets.cmake
  NAMESPACE UniversalRng::
  DESTINATION "${UA_INSTALL_CMAKEDIR}")

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/UniversalRngConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/UniversalRngConfigVersion.cmake"
  DESTINATION "${UA_INSTALL_CMAKEDIR}")

# ----------------- pkg-config (optional) -----------------
option(UA_INSTALL_PKGCONFIG "Install pkg-config file" ON)
if (UA_INSTALL_PKGCONFIG)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/UniversalRng.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/UniversalRng.pc"
    @ONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/UniversalRng.pc" DESTINATION lib/pkgconfig)
endif()

# ----------------- benches (existing) -----------------
if (ENABLE_BENCH)
  add_executable(rng_lockbench bench/bench_lock_and_bench.cpp)
  target_link_libraries(rng_lockbench PRIVATE universal_rng_static)
  target_compile_features(rng_lockbench PRIVATE cxx_std_20)

  add_executable(rng_compare bench/bench_compare_rngs.cpp)
  target_link_libraries(rng_compare PRIVATE universal_rng_static)
  target_compile_features(rng_compare PRIVATE cxx_std_20)

  add_executable(rng_scale bench/bench_scale.cpp)
  target_link_libraries(rng_scale PRIVATE universal_rng_static)
  target_compile_features(rng_scale PRIVATE cxx_std_20)
endif()
