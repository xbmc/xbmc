#.rst:
# FindWebOSHelpers
# --------
# Finds the WebOSHelpers library
#
# This will define the following variables::
#
# WEBOSHELPERS_FOUND - system has WebOSHelpers
# WEBOSHELPERS_INCLUDE_DIRS - the WebOSHelpers include directory
# WEBOSHELPERS_LIBRARIES - the WebOSHelpers libraries
# WEBOSHELPERS_DEFINITIONS - the WebOSHelpers compile definitions
#
# and the following imported targets::
#
#   WEBOSHELPERS::WEBOSHELPERS   - The webOS helpers library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_WEBOSHELPERS helpers>=2.0.0 QUIET)
endif()

find_path(WEBOSHELPERS_INCLUDE_DIR NAMES webos-helpers/libhelpers.h
        PATHS ${PC_WEBOSHELPERS_INCLUDEDIR})
find_library(WEBOSHELPERS_LIBRARY NAMES helpers
        PATHS ${PC_WEBOSHELPERS_LIBDIR})

set(WEBOSHELPERS_VERSION ${PC_WEBOSHELPERS_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WebOSHelpers
                                  REQUIRED_VARS WEBOSHELPERS_LIBRARY WEBOSHELPERS_INCLUDE_DIR
                                  VERSION_VAR WEBOSHELPERS_VERSION)

if(WEBOSHELPERS_FOUND)
  set(WEBOSHELPERS_INCLUDE_DIRS ${WEBOSHELPERS_INCLUDE_DIR})
  set(WEBOSHELPERS_LIBRARIES ${WEBOSHELPERS_LIBRARY})

  if(NOT TARGET WEBOSHELPERS::WEBOSHELPERS)
    add_library(WEBOSHELPERS::WEBOSHELPERS UNKNOWN IMPORTED)
    set_target_properties(WEBOSHELPERS::WEBOSHELPERS PROPERTIES
                                     IMPORTED_LOCATION "${WEBOSHELPERS_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${WEBOSHELPERS_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(WEBOSHELPERS_INCLUDE_DIR WEBOSHELPERS_LIBRARY)
