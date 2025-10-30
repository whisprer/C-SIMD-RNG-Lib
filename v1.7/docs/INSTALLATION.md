/docs/INSTALLATION.md
# UA RNG v1.7 – Installation Guide

UA RNG is a fast, cross-platform C++20 random number library with runtime SIMD dispatch.  
This guide covers installation on **Linux**, **macOS**, and **Windows (MSVC/MinGW)**.

---

## 1. Prerequisites

- **C++20 compiler**  
  - GCC 10+  
  - Clang 12+  
  - MSVC 2019+ (with AVX2/AVX-512 enabled if available)

- **CMake 3.16+**

- **Git** (if building from repository)

- **Platform tools**
  - Linux: `build-essential`, `cmake`, `ninja-build`
  - macOS: Xcode Command Line Tools, `brew install cmake ninja`
  - Windows: Visual Studio 2019+ with CMake support, or MSYS2/MinGW toolchain

---

## 2. Build Instructions

### Linux / macOS

```bash
git clone https://github.com/yourname/ua_rng.git
cd ua_rng
mkdir build && cd build

cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DUA_ENABLE_AVX2=ON \
         -DUA_ENABLE_AVX512=ON

cmake --build . --parallel
ctest                # optional: run tests
sudo cmake --install .

Windows – MSVC (Developer PowerShell)
git clone https://github.com/yourname/ua_rng.git
cd ua_rng
mkdir build; cd build

cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
ctest -C Release
cmake --install . --config Release --prefix "C:\Program Files\ua_rng"

Windows – MinGW/MSYS2
git clone https://github.com/yourname/ua_rng.git
cd ua_rng
mkdir build && cd build

cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release \
         -DUA_ENABLE_AVX2=ON -DUA_ENABLE_AVX512=ON
cmake --build . --parallel
ctest

3. Output Artifacts
Linux

Shared: lib/libua_rng.so (versioned symlink supported)

Static (optional): lib/libua_rng.a

Headers: include/ua/*.h

Pkgconfig: lib/pkgconfig/ua_rng.pc (if enabled)

macOS

Shared: lib/libua_rng.1.7.dylib + libua_rng.dylib symlink

Static (optional): lib/libua_rng.a

Headers: include/ua/*.h

Windows (MSVC)

Shared: bin/ua_rng.dll + lib/ua_rng.lib (import lib)

Static (optional): lib/ua_rng.lib

Headers: include/ua/*.h

Windows (MinGW-w64)

Shared: bin/libua_rng.dll + lib/libua_rng.dll.a

Static (optional): lib/libua_rng.a

Headers: include/ua/*.h

4. Using UA RNG in Your Project

In your project’s CMakeLists.txt:

add_subdirectory(external/ua_rng)
target_link_libraries(my_app PRIVATE ua_rng)

target_include_directories(my_app PRIVATE
    ${PROJECT_SOURCE_DIR}/external/ua_rng/include)


Example:

#include "ua/ua_rng.h"

int main() {
    ua::Rng rng(42);
    uint64_t val = rng.generate_u64();
    double d = rng.generate_double();
}

5. Notes

Runtime dispatch means one binary runs everywhere: SIMD code paths are only used when safe.

For performance tuning, set environment variable UA_FORCE_BACKEND to scalar, avx2, or avx512.

Use UA_STREAM_STORES=1 to enable non-temporal stores when buffers are properly aligned.


---
