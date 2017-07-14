if(NOT CORE_SYSTEM_NAME)
  string(TOLOWER ${CMAKE_SYSTEM_NAME} CORE_SYSTEM_NAME)
endif()

if(CORE_SYSTEM_NAME STREQUAL linux)
  # Set default CORE_PLATFORM_NAME to X11
  # This is overridden by user setting -DCORE_PLATFORM_NAME=<platform>
  set(_DEFAULT_PLATFORM X11)
else()
  string(TOLOWER ${CORE_SYSTEM_NAME} _DEFAULT_PLATFORM)
endif()

#
# Note: please do not use CORE_PLATFORM_NAME in any checks,
# use the normalized to lower case CORE_PLATFORM_NAME_LC (see below) instead
#
if(NOT CORE_PLATFORM_NAME)
  set(CORE_PLATFORM_NAME ${_DEFAULT_PLATFORM} CACHE STRING "Platform port to build")
endif()
unset(_DEFAULT_PLATFORM)
string(TOLOWER ${CORE_PLATFORM_NAME} CORE_PLATFORM_NAME_LC)

list(APPEND final_message "Platform: ${CORE_PLATFORM_NAME}")
if(EXISTS ${CMAKE_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/${CORE_PLATFORM_NAME_LC}.cmake)
  include(${CMAKE_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/${CORE_PLATFORM_NAME_LC}.cmake)
else()
  file(GLOB _platformnames RELATIVE ${CMAKE_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/
                                    ${CMAKE_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/*.cmake)
  string(REPLACE ".cmake" " " _platformnames ${_platformnames})
  message(FATAL_ERROR "invalid CORE_PLATFORM_NAME: ${CORE_PLATFORM_NAME_LC}\nValid platforms: ${_platformnames}")
endif()

