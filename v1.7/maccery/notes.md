a truly universal library (works on old SSE2-only boxes up to big AVX-512 servers) and for us to start “digging into the dirty bits.” below is a clean, complete, buildable v1.7 drop-in that gives you:

scalar baseline (no intrinsics) = runs anywhere

AVX2 backend (4× lanes)

optional AVX-512F backend (8× lanes)

runtime dispatch (no ifunc), safe on any CPU

per-TU compile flags (so you can ship one portable lib)

fast u64 and \[0,1) double streams

normal variates via Marsaglia Polar (scalar + AVX2)

simple bench that reports ms \& cycles/elem

tidy CMake with options for AVX2/AVX-512, stream stores, LTO

no placeholders, no TODOs, just working code







macOS (Clang)



Static



lib/libua\_rng.a



Shared



lib/libua\_rng.dylib (or versioned libua\_rng.1.7.dylib + symlink)



(optional) .dSYM bundle for symbols











macOS (Clang)



Static



lib/libua\_rng.a



Shared



lib/libua\_rng.dylib (or versioned libua\_rng.1.7.dylib + symlink)



(optional) .dSYM bundle for symbols











Example: macOS (both)

ua\_rng-1.7/

&nbsp; include/ua/\*.h

&nbsp; lib/libua\_rng.1.7.dylib

&nbsp; lib/libua\_rng.dylib      # symlink to versioned

&nbsp; lib/libua\_rng.a











Linux/macOS builds (when you do them):

Headers: dist/include/ua/\*.h

Static: dist/lib/libua\_rng.a

Shared: dist/lib/libua\_rng.so\[.\*] (Linux) or dist/lib/libua\_rng\*.dylib (macOS)

Optional: dist/lib/pkgconfig/ua\_rng.pc

Copy those into your clean ua\_rng-1.7/ bundle as shown.









10-second sanity checks users can run

macOS (shared):



otool -L your\_app shows @rpath/libua\_rng.dylib



Adjust rpath or use install\_name\_tool if needed.















libua\_rng.1.7.dylib (macOS, versioned):



Purpose: versioned install name + symlink libua\_rng.dylib → libua\_rng.1.7.dylib. Helps package managers and multiple versions to coexist cleanly.



If you don’t ship macOS binaries right now, you won’t have this yet.



ua\_rng.pc (pkg-config file, Linux/macOS):



✅ Optional. Only helpful for users who build with pkg-config --cflags --libs ua\_rng. If your consumers use CMake find\_package(ua\_rng) or include headers + link manually, they don’t need it.



So: nothing “missing” that blocks people. These are conveniences.



How to produce them (when you’re on macOS/Linux)

1\) Versioned .dylib on macOS



Add this once to your CMake (near where you define the shared lib target):



\# after add\_library(ua\_rng\_shared SHARED ...)

set\_target\_properties(ua\_rng\_shared PROPERTIES

  OUTPUT\_NAME ua\_rng

  VERSION 1.7           # full library version

  SOVERSION 1           # ABI major

  FRAMEWORK FALSE       # (ensure it’s a .dylib, not a framework)

)





Configure \& build on a Mac:



cmake -S . -B build-macos -G "Ninja" \\

  -DCMAKE\_BUILD\_TYPE=Release -DUA\_BUILD\_SHARED=ON -DUA\_BUILD\_STATIC=OFF

cmake --build build-macos -j

cmake --install build-macos --prefix "$(pwd)/dist-macos"





You’ll get:



dist-macos/lib/libua\_rng.1.7.dylib

dist-macos/lib/libua\_rng.1.dylib      -> symlink to 1.7

dist-macos/lib/libua\_rng.dylib        -> symlink to 1.7



2\) ua\_rng.pc (pkg-config) on Linux/macOS



Add this once to your repo:



cmake/ua\_rng.pc.in



prefix=@CMAKE\_INSTALL\_PREFIX@

exec\_prefix=${prefix}

libdir=${prefix}/lib

includedir=${prefix}/include



Name: ua\_rng

Description: Universal Architecture RNG library

Version: @PROJECT\_VERSION@

Libs: -L${libdir} -lua\_rng

Cflags: -I${includedir}





CMakeLists.txt (install section)



include(CMakePackageConfigHelpers)

\# if you have a project() with VERSION set, this works; else hardcode 1.7

set(PROJECT\_VERSION 1.7)



configure\_file(

  ${CMAKE\_CURRENT\_SOURCE\_DIR}/cmake/ua\_rng.pc.in

  ${CMAKE\_CURRENT\_BINARY\_DIR}/ua\_rng.pc

  @ONLY

)

install(FILES ${CMAKE\_CURRENT\_BINARY\_DIR}/ua\_rng.pc

        DESTINATION lib/pkgconfig)







macOS



include/ua/\*.h



Shared: lib/libua\_rng.1.7.dylib + symlink libua\_rng.dylib



Static (optional): lib/libua\_rng.a



TL;DR



You’re not blocked: those two files are optional conveniences.



Add the small CMake bits above and build on the respective OS when you’re ready, and they’ll appear.



The platform set you listed (MSVC, MSYS2/MinGW, Linux, macOS) is exactly what most users expect.



You said:

i found ua\_rng.lib which doesn't see to be required by any one, but i \_can't\_ find libua\_rng.so for linux or lib/libua\_rng.1.7.dylib 'n' libua\_rng.dylib for macOS, how tf do i get htese agoian  and do i have to do summin weir di dint do by deault ot get thebasi windows ones?

ChatGPT said:



gotcha fren — let’s break this down real clean:



why you don’t see libua\_rng.so or .dylib



on Windows (MSVC / MinGW):



you get .dll for shared, .lib (MSVC import or static), .a (MinGW import or static).



✅ these are normal, you did nothing wrong.



on Linux/macOS:



CMake will only build the shared .so / .dylib if BUILD\_SHARED\_LIBS=ON or your custom UA\_BUILD\_SHARED=ON is handled in the CMakeLists.



if your current CMake doesn’t install them, you’ll only see the Windows outputs.



so the missing .so/.dylib is just because you haven’t built on those OSes (or cross-compiled with the right flags).



how to get them

Linux

mkdir build-linux \&\& cd build-linux

cmake .. -DCMAKE\_BUILD\_TYPE=Release -DUA\_BUILD\_SHARED=ON -DUA\_BUILD\_STATIC=OFF

cmake --build . -j

cmake --install . --prefix ../dist-linux





you’ll get:



dist-linux/include/ua/ua\_rng.h

dist-linux/lib/libua\_rng.so.1.7   # versioned soname

dist-linux/lib/libua\_rng.so       # symlink



macOS

mkdir build-macos \&\& cd build-macos

cmake .. -DCMAKE\_BUILD\_TYPE=Release -DUA\_BUILD\_SHARED=ON -DUA\_BUILD\_STATIC=OFF

cmake --build . -j

cmake --install . --prefix ../dist-macos





if you’ve got this in CMakeLists.txt:



set\_target\_properties(ua\_rng\_shared PROPERTIES

  OUTPUT\_NAME ua\_rng

  VERSION 1.7

  SOVERSION 1

)





you’ll see:



dist-macos/lib/libua\_rng.1.7.dylib

dist-macos/lib/libua\_rng.1.dylib   -> symlink

dist-macos/lib/libua\_rng.dylib     -> symlink



do you need to do anything special for Windows?



nope.



MSVC: .dll (shared) + .lib (import or static).



MinGW: .dll (shared) + .dll.a (import) or .a (static).



CMake generates those by default when you set UA\_BUILD\_SHARED / UA\_BUILD\_STATIC.



tl;dr for you



the Linux .so and macOS .dylib won’t ever appear unless you build on those OSes (or cross-compile).



your Windows .lib/.dll is already correct and expected.



if you want the macOS/Linux artifacts, either build them natively on those platforms or set up a cross-compile toolchain.



👉 fren, do you want me to write you CMake tweaks that guarantee .so and .dylib get the right versioned names, so when you next build on Linux/macOS they’ll just show up in dist-\*/lib/ automatically?







A) Easiest + clean: build on macOS via GitHub Actions



This avoids SDK/toolchain drama on your Linux box and gives you signed, legit macOS artifacts for both x86\_64 and arm64.



Create .github/workflows/macos.yml in your repo:





When the workflow finishes, download the artifact ua\_rng-macos-universal. Inside you’ll have:





This is the recommended way.

