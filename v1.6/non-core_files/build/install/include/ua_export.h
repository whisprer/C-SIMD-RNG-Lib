// include/ua_export.h
#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
  #if defined(UA_BUILD_SHARED)
    #define UA_API __declspec(dllexport)
  #elif defined(UA_USE_SHARED)
    #define UA_API __declspec(dllimport)
  #else
    #define UA_API
  #endif
#else
  #if __GNUC__ >= 4
    #define UA_API __attribute__((visibility("default")))
  #else
    #define UA_API
  #endif
#endif
