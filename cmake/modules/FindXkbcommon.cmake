# FindXkbcommon
# -----------
# Finds the libxkbcommon library
#
# This will define the following variables::
#
# XKBCOMMON_FOUND        - the system has libxkbcommon
# XKBCOMMON_INCLUDE_DIRS - the libxkbcommon include directory
# XKBCOMMON_LIBRARIES    - the libxkbcommon libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_XKBCOMMON xkbcommon QUIET)
endif()


find_path(XKBCOMMON_INCLUDE_DIR NAMES xkbcommon/xkbcommon.h
                           PATHS ${PC_XKBCOMMON_INCLUDEDIR})
find_library(XKBCOMMON_LIBRARY NAMES xkbcommon
                          PATHS ${PC_XKBCOMMON_LIBDIR})

set(XKBCOMMON_VERSION ${PC_XKBCOMMON_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Xkbcommon
                                  REQUIRED_VARS XKBCOMMON_LIBRARY XKBCOMMON_INCLUDE_DIR
                                  VERSION_VAR XKBCOMMON_VERSION)

if(XKBCOMMON_FOUND)
  set(XKBCOMMON_INCLUDE_DIRS ${XKBCOMMON_INCLUDE_DIR})
  set(XKBCOMMON_LIBRARIES ${XKBCOMMON_LIBRARY})

  if(NOT TARGET XKBCOMMON::XKBCOMMON)
    add_library(XKBCOMMON::XKBCOMMON UNKNOWN IMPORTED)
    set_target_properties(XKBCOMMON::XKBCOMMON PROPERTIES
                                     IMPORTED_LOCATION "${XKBCOMMON_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${XKBCOMMON_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(XKBCOMMON_INCLUDE_DIR XKBCOMMON_LIBRARY)
