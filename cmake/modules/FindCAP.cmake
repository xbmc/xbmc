#.rst:
# FindCAP
# -----------
# Finds the POSIX 1003.1e capabilities library
#
# This will define the following variables::
#
# CAP_FOUND - system has LibCap
# CAP_INCLUDE_DIRS - the LibCap include directory
# CAP_LIBRARIES - the LibCap libraries
#
# and the following imported targets::
#
# CAP::CAP - The LibCap library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CAP libcap QUIET)
endif()

find_path(CAP_INCLUDE_DIR NAMES sys/capability.h
                          PATHS ${PC_CAP_INCLUDEDIR})
find_library(CAP_LIBRARY NAMES cap libcap
                         PATHS ${PC_CAP_LIBDIR})

set(CAP_VERSION ${PC_CAP_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CAP
                                  REQUIRED_VARS CAP_LIBRARY CAP_INCLUDE_DIR
                                  VERSION_VAR CAP_VERSION)

if(CAP_FOUND)
  set(CAP_LIBRARIES ${CAP_LIBRARY})
  set(CAP_INCLUDE_DIRS ${CAP_INCLUDE_DIR})

  if(NOT TARGET CAP::CAP)
    add_library(CAP::CAP UNKNOWN IMPORTED)
    set_target_properties(CAP::CAP PROPERTIES
                                   IMPORTED_LOCATION "${CAP_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${CAP_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(CAP_INCLUDE_DIR CAP_LIBRARY)
