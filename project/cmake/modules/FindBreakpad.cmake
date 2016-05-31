#.rst:
# FindBreakpad
# ------------
# Finds the Breakpad library
#
# This will will define the following variables::
#
# BREAKPAD_FOUND - system has Breakpad
# BREAKPAD_INCLUDE_DIRS - the Breakpad include directory
# BREAKPAD_LIBRARIES - the Breakpad libraries
#
# and the following imported targets::
#
#   Breakpad::Breakpad   - The Breakpad library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_BREAKPAD breakpad-client QUIET)
endif()

find_path(BREAKPAD_INCLUDE_DIR google_breakpad/common/breakpad_types.h
                              PATH_SUFFIXES breakpad
                              PATHS ${PC_BREAKPAD_INCLUDEDIR})
find_library(BREAKPAD_LIBRARY NAMES breakpad_client
                              PATHS ${PC_BREAKPAD_LIBDIR})
set(BREAKPAD_VERSION ${PC_BREAKPAD_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Breakpad
                                  REQUIRED_VARS BREAKPAD_LIBRARY BREAKPAD_INCLUDE_DIR
                                  VERSION_VAR BREAKPAD_VERSION)

if(BREAKPAD_FOUND)
  set(BREAKPAD_LIBRARIES ${BREAKPAD_LIBRARY})
  set(BREAKPAD_INCLUDE_DIRS ${BREAKPAD_INCLUDE_DIR})

  if(NOT TARGET Breakpad::Breakpad)
    add_library(Breakpad::Breakpad UNKNOWN IMPORTED)
    set_target_properties(Breakpad::Breakpad PROPERTIES
                                             IMPORTED_LOCATION "${BREAKPAD_LIBRARY}"
                                             INTERFACE_INCLUDE_DIRECTORIES "${BREAKPAD_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(BREAKPAD_INCLUDE_DIR BREAKPAD_LIBRARY)
