add_library(ua_rng STATIC ${UA_SOURCES})
add_library(ua_rng_shared SHARED ${UA_SOURCES})

set_target_properties(ua_rng PROPERTIES OUTPUT_NAME "ua_rng")
set_target_properties(ua_rng_shared PROPERTIES OUTPUT_NAME "ua_rng")

target_include_directories(ua_rng PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(ua_rng_shared PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_features(ua_rng PUBLIC cxx_std_20)
target_compile_features(ua_rng_shared PUBLIC cxx_std_20)

# Export symbols for Windows DLL
if (WIN32)
  target_compile_definitions(ua_rng_shared PRIVATE UA_DLL_EXPORTS)
endif()
