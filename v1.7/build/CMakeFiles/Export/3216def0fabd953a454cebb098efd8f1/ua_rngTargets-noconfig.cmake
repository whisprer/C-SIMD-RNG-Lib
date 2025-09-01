#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ua::ua_rng" for configuration ""
set_property(TARGET ua::ua_rng APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(ua::ua_rng PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libua_rng.a"
  )

list(APPEND _cmake_import_check_targets ua::ua_rng )
list(APPEND _cmake_import_check_files_for_ua::ua_rng "${_IMPORT_PREFIX}/lib/libua_rng.a" )

# Import target "ua::ua_rng_shared" for configuration ""
set_property(TARGET ua::ua_rng_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(ua::ua_rng_shared PROPERTIES
  IMPORTED_IMPLIB_NOCONFIG "${_IMPORT_PREFIX}/lib/libua_rng.dll.a"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/bin/libua_rng.dll"
  )

list(APPEND _cmake_import_check_targets ua::ua_rng_shared )
list(APPEND _cmake_import_check_files_for_ua::ua_rng_shared "${_IMPORT_PREFIX}/lib/libua_rng.dll.a" "${_IMPORT_PREFIX}/bin/libua_rng.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
