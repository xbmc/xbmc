#.rst:
# FindFribidi
# -----------
# Finds the GNU FriBidi library
#
# This will define the following variables::
#
# FRIBIDI_FOUND - system has FriBidi
# FRIBIDI_INCLUDE_DIRS - the FriBidi include directory
# FRIBIDI_LIBRARIES - the FriBidi libraries
#
# and the following imported targets::
#
#   FriBidi::FriBidi   - The FriBidi library

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_FRIBIDI fribidi QUIET)
endif()

find_path(FRIBIDI_INCLUDE_DIR NAMES fribidi.h
                              PATH_SUFFIXES fribidi
                              HINTS ${PC_FRIBIDI_INCLUDEDIR})
find_library(FRIBIDI_LIBRARY NAMES fribidi libfribidi
                             HINTS ${PC_FRIBIDI_LIBDIR})

set(FRIBIDI_VERSION ${PC_FRIBIDI_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FriBidi
                                  REQUIRED_VARS FRIBIDI_LIBRARY FRIBIDI_INCLUDE_DIR
                                  VERSION_VAR FRIBIDI_VERSION)

if(FRIBIDI_FOUND)
  set(FRIBIDI_LIBRARIES ${FRIBIDI_LIBRARY})
  set(FRIBIDI_INCLUDE_DIRS ${FRIBIDI_INCLUDE_DIR})
  if(PC_FRIBIDI_INCLUDE_DIRS)
    list(APPEND FRIBIDI_INCLUDE_DIRS ${PC_FRIBIDI_INCLUDE_DIRS})
  endif()
  if(PC_FRIBIDI_CFLAGS_OTHER)
    set(FRIBIDI_DEFINITIONS ${PC_FRIBIDI_CFLAGS_OTHER})
  endif()

  if(NOT TARGET FriBidi::FriBidi)
    add_library(FriBidi::FriBidi UNKNOWN IMPORTED)
    set_target_properties(FriBidi::FriBidi PROPERTIES
                                           IMPORTED_LOCATION "${FRIBIDI_LIBRARY}"
                                           INTERFACE_INCLUDE_DIRECTORIES "${FRIBIDI_INCLUDE_DIRS}"
                                           INTERFACE_COMPILE_OPTIONS "${FRIBIDI_DEFINITIONS}")
  endif()
endif()

mark_as_advanced(FRIBIDI_INCLUDE_DIR FRIBIDI_LIBRARY)
