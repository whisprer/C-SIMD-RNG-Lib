cmake_minimum_required(VERSION 3.20)
project(ua_rng VERSION 1.7.0 LANGUAGES CXX)

# ---- options (these make your -D flags "used") ----
option(UA_ENABLE_AVX2   "Build AVX2 backend" ON)
option(UA_ENABLE_AVX512 "Build AVX-512F backend" ON)
option(UA_BUILD_STATIC  "Build static library" ON)
option(UA_BUILD_SHARED  "Build shared library" ON)
option(UA_BUILD_BENCH   "Build bench app" OFF)
option(UA_BUILD_MSVC_TEST "Build tiny MSVC sanity test" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- core sources ----
set(UA_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ua_cpuid.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ua_rng.cpp
)

if (UA_ENABLE_AVX2)
  list(APPEND UA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/xoshiro256ss_avx2.cpp)
endif()
if (UA_ENABLE_AVX512)
  list(APPEND UA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/xoshiro256ss_avx512.cpp)
endif()

# ---- libraries ----
set(UA_PUBLIC_INC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (UA_BUILD_STATIC)
  add_library(ua_rng STATIC ${UA_SOURCES})
  target_include_directories(ua_rng PUBLIC ${UA_PUBLIC_INC})
  target_compile_definitions(ua_rng PUBLIC
    $<$<BOOL:${UA_ENABLE_AVX2}>:UA_BUILD_WITH_AVX2=1>
    $<$<BOOL:${UA_ENABLE_AVX512}>:UA_BUILD_WITH_AVX512=1>
  )
endif()

if (UA_BUILD_SHARED)
  add_library(ua_rng_shared SHARED ${UA_SOURCES})
  target_include_directories(ua_rng_shared PUBLIC ${UA_PUBLIC_INC})
  target_compile_definitions(ua_rng_shared PUBLIC
    $<$<BOOL:${UA_ENABLE_AVX2}>:UA_BUILD_WITH_AVX2=1>
    $<$<BOOL:${UA_ENABLE_AVX512}>:UA_BUILD_WITH_AVX512=1>
    UA_RNG_EXPORTS
  )
  set_target_properties(ua_rng_shared PROPERTIES OUTPUT_NAME ua_rng)
endif()

# ---- optional tiny MSVC test (OFF by default) ----
if (UA_BUILD_MSVC_TEST)
  set(_UA_MSVC_TEST ${CMAKE_CURRENT_SOURCE_DIR}/tests/msvc_test.cpp)
  if (EXISTS ${_UA_MSVC_TEST})
    add_executable(msvc_test ${_UA_MSVC_TEST})
    target_include_directories(msvc_test PRIVATE ${UA_PUBLIC_INC})
    # link whichever you built; here we prefer the shared:
    if (TARGET ua_rng_shared)
      target_link_libraries(msvc_test PRIVATE ua_rng_shared)
    else()
      target_link_libraries(msvc_test PRIVATE ua_rng)
    endif()
  else()
    message(FATAL_ERROR "UA_BUILD_MSVC_TEST=ON but tests/msvc_test.cpp not found")
  endif()
endif()

# ---- install rules (this creates INSTALL.vcxproj) ----
include(GNUInstallDirs)
set(INSTALL_INC_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(INSTALL_LIB_DIR ${CMAKE_INSTALL_LIBDIR})
set(INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})

if (TARGET ua_rng)
  install(TARGETS ua_rng
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${INSTALL_BIN_DIR}
  )
endif()
if (TARGET ua_rng_shared)
  install(TARGETS ua_rng_shared
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${INSTALL_BIN_DIR}
  )
endif()
install(DIRECTORY ${UA_PUBLIC_INC}/ DESTINATION ${INSTALL_INC_DIR})
