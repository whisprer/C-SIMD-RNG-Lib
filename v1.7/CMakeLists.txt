cmake_minimum_required(VERSION 3.20)
project(ua_consumer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Adjust these two if you relocate dist-msvc
set(UA_DIST "D:/code/Universal-Architecture-RNG-Lib/v1.7/dist-msvc")
set(UA_INC  "${UA_DIST}/include")
set(UA_LIB  "${UA_DIST}/lib")

add_executable(msvc_test msvc_test.cpp)
target_include_directories(msvc_test PRIVATE "${UA_INC}")
target_link_directories(msvc_test PRIVATE "${UA_LIB}")
target_link_libraries(msvc_test PRIVATE ua_rng)
