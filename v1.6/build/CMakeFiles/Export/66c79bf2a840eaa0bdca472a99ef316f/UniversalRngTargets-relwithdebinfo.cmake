#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "UniversalRng::universal_rng_shared" for configuration "RelWithDebInfo"
set_property(TARGET UniversalRng::universal_rng_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(UniversalRng::universal_rng_shared PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/universal_rng.lib"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/universal_rng.dll"
  )

list(APPEND _cmake_import_check_targets UniversalRng::universal_rng_shared )
list(APPEND _cmake_import_check_files_for_UniversalRng::universal_rng_shared "${_IMPORT_PREFIX}/lib/universal_rng.lib" "${_IMPORT_PREFIX}/bin/universal_rng.dll" )

# Import target "UniversalRng::universal_rng_static" for configuration "RelWithDebInfo"
set_property(TARGET UniversalRng::universal_rng_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(UniversalRng::universal_rng_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/universal_rng.lib"
  )

list(APPEND _cmake_import_check_targets UniversalRng::universal_rng_static )
list(APPEND _cmake_import_check_files_for_UniversalRng::universal_rng_static "${_IMPORT_PREFIX}/lib/universal_rng.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
