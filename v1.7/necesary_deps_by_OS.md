Windows / MSVC
	include/ua/*.h
	Shared: bin/ua_rng.dll + lib/ua_rng.lib (import lib)
	Static (optional): lib/ua_rng.lib (built as static)

Windows / MinGW-w64
	include/ua/*.h
	Shared: bin/libua_rng.dll + lib/libua_rng.dll.a
	Static (optional): lib/libua_rng.a

Linux
	include/ua/*.h
	Shared: lib/libua_rng.so (or versioned + symlink)
	Static (optional): lib/libua_rng.a
	Optional: lib/pkgconfig/ua_rng.pc

macOS
	include/ua/*.h
	Shared: lib/libua_rng.1.7.dylib + symlink libua_rng.dylib
	Static (optional): lib/libua_rng.a
