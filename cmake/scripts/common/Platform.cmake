if(NOT CORE_SYSTEM_NAME)
  string(TOLOWER ${CMAKE_SYSTEM_NAME} CORE_SYSTEM_NAME)
endif()
# Switch used path, if CORE_SOURCE_DIR is set use it (e.g. on addons build)
# otherwise use the present source dir
#
# TODO: This should be refactored on v19 and the if usage removed!
if(CORE_SOURCE_DIR)
  set(PLATFORM_USED_SOURCE_DIR ${CORE_SOURCE_DIR})
else()
  set(PLATFORM_USED_SOURCE_DIR ${CMAKE_SOURCE_DIR})
endif()

if(CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd)
  # Set default CORE_PLATFORM_NAME to X11
  # This is overridden by user setting -DCORE_PLATFORM_NAME=<platform>
  set(_DEFAULT_PLATFORM X11)
  option(ENABLE_APP_AUTONAME    "Enable renaming the binary according to windowing?" ON)
else()
  string(TOLOWER ${CORE_SYSTEM_NAME} _DEFAULT_PLATFORM)
endif()

set(APP_BINARY_SUFFIX ".bin")

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
if(EXISTS ${PLATFORM_USED_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/${CORE_PLATFORM_NAME_LC}.cmake)
  include(${PLATFORM_USED_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/${CORE_PLATFORM_NAME_LC}.cmake)
  if(ENABLE_APP_AUTONAME)
    set(APP_BINARY_SUFFIX "-${CORE_PLATFORM_NAME_LC}")
  endif()
else()
  file(GLOB _platformnames RELATIVE ${PLATFORM_USED_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/
                                    ${PLATFORM_USED_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/*.cmake)
  string(REPLACE ".cmake" " " _platformnames ${_platformnames})
  message(FATAL_ERROR "invalid CORE_PLATFORM_NAME: ${CORE_PLATFORM_NAME_LC}\nValid platforms: ${_platformnames}")
endif()

unset(PLATFORM_USED_SOURCE_DIR)
