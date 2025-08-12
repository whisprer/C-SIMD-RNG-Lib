
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was UniversalRngConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include("${CMAKE_CURRENT_LIST_DIR}/UniversalRngTargets.cmake")

# Provide a simple alias for consumers:
#   find_package(UniversalRng REQUIRED)
#   target_link_libraries(app PRIVATE UniversalRng::universal_rng)
# The alias will prefer the shared lib if present, else static.
if (TARGET UniversalRng::universal_rng)
  # nothing
elseif (TARGET UniversalRng::universal_rng_shared)
  add_library(UniversalRng::universal_rng ALIAS UniversalRng::universal_rng_shared)
elseif (TARGET UniversalRng::universal_rng_static)
  add_library(UniversalRng::universal_rng ALIAS UniversalRng::universal_rng_static)
endif()
