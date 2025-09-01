Windows / MSVC
include/ua/\*.h
Shared: bin/ua\_rng.dll + lib/ua\_rng.lib (import lib)
Static (optional): lib/ua\_rng.lib (built as static)



Windows / MinGW-w64
include/ua/\*.h
Shared: bin/libua\_rng.dll + lib/libua\_rng.dll.a
Static (optional): lib/libua\_rng.a



Linux
include/ua/\*.h
Shared: lib/libua\_rng.so (or versioned + symlink)
Static (optional): lib/libua\_rng.a
Optional: lib/pkgconfig/ua\_rng.pc



macOS
include/ua/\*.h
Shared: lib/libua\_rng.1.7.dylib + symlink libua\_rng.dylib
Static (optional): lib/libua\_rng.a

