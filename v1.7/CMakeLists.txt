cmake_minimum_required(VERSION 3.20)
project(ua_rng LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
  add_compile_options(/O2)
else()
  add_compile_options(-O3 -fno-exceptions -fno-rtti -fno-omit-frame-pointer -march=native -mtune=native -flto)
endif()

add_library(ua_rng INTERFACE)
target_include_directories(ua_rng INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(ua_rng_bench bench/bench_main.cpp)
target_link_libraries(ua_rng_bench PRIVATE ua_rng)
