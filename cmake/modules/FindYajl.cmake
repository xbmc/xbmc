#.rst:
# FindYajl
# --------
# Finds the Yajl library
#
# This will will define the following variables::
#
# YAJL_FOUND - system has Yajl
# YAJL_INCLUDE_DIRS - Yajl include directory
# YAJL_LIBRARIES - the Yajl libraries
#
# and the following imported targets::
#
#   Yajl::Yajl   - The Yajl library

if(NOT Yajl_FIND_VERSION)
  set(Yajl_FIND_VERSION 2.0.0)
endif()

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_YAJL yajl>=${Yajl_FIND_VERSION} QUIET)
endif()

find_path(YAJL_INCLUDE_DIR NAMES yajl/yajl_common.h
                           PATHS ${PC_YAJL_INCLUDEDIR})
find_library(YAJL_LIBRARY NAMES yajl
                          PATHS ${PC_YAJL_LIBDIR})

if(PC_YAJL_VERSION)
  set(YAJL_VERSION_STRING ${PC_YAJL_VERSION})
elseif(YAJL_INCLUDE_DIR AND EXISTS "${YAJL_INCLUDE_DIR}/yajl/yajl_version.h")
  file(STRINGS "${YAJL_INCLUDE_DIR}/yajl/yajl_version.h" yajl_version_str REGEX "^[ \t]*#define[ \t]+YAJL_(MAJOR|MINOR|MICRO)")
  string(REGEX REPLACE "YAJL_MAJOR ([0-9]+)" "\\1" YAJL_VERSION_MAJOR "${YAJL_VERSION_MAJOR}")

  string(REGEX REPLACE ".*YAJL_MAJOR ([0-9]+).*" "\\1" yajl_major "${yajl_version_str}")
  string(REGEX REPLACE ".*YAJL_MINOR ([0-9]+).*" "\\1" yajl_minor "${yajl_version_str}")
  string(REGEX REPLACE ".*YAJL_MICRO ([0-9]+).*" "\\1" yajl_micro "${yajl_version_str}")
  set(YAJL_VERSION_STRING "${yajl_major}.${yajl_minor}.${yajl_micro}")
  unset(yajl_version_str)
  unset(yajl_major)
  unset(yajl_minor)
  unset(yajl_micro)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Yajl
                                  REQUIRED_VARS YAJL_LIBRARY YAJL_INCLUDE_DIR
                                  VERSION_VAR YAJL_VERSION_STRING)

if(YAJL_FOUND)
  set(YAJL_INCLUDE_DIRS ${YAJL_INCLUDE_DIR})
  set(YAJL_LIBRARIES ${YAJL_LIBRARY})

  if(NOT TARGET Yajl::Yajl)
    add_library(Yajl::Yajl UNKNOWN IMPORTED)
    set_target_properties(Yajl::Yajl PROPERTIES
                                     IMPORTED_LOCATION "${YAJL_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${YAJL_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(YAJL_INCLUDE_DIR YAJL_LIBRARY)
