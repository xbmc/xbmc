#.rst:
# FindHarfbuzz
# ------------
# Finds the HarfBuzz library
#
# This will define the following variables::
#
# HARFBUZZ_FOUND - system has HarfBuzz
# HARFBUZZ_INCLUDE_DIRS - the HarfBuzz include directory
# HARFBUZZ_LIBRARIES - the HarfBuzz libraries
#
# and the following imported targets::
#
#   HarfBuzz::HarfBuzz   - The HarfBuzz library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_HARFBUZZ harfbuzz QUIET)
endif()

find_path(HARFBUZZ_INCLUDE_DIR NAMES harfbuzz/hb-ft.h hb-ft.h
                               PATHS ${PC_HARFBUZZ_INCLUDEDIR}
                                     ${PC_HARFBUZZ_INCLUDE_DIRS}
                               PATH_SUFFIXES harfbuzz)
find_library(HARFBUZZ_LIBRARY NAMES harfbuzz harfbuzz
                              PATHS ${PC_HARFBUZZ_LIBDIR})

set(HARFBUZZ_VERSION ${PC_HARFBUZZ_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HarfBuzz
                                  REQUIRED_VARS HARFBUZZ_LIBRARY HARFBUZZ_INCLUDE_DIR
                                  VERSION_VAR HARFBUZZ_VERSION)

if(HARFBUZZ_FOUND)
  set(HARFBUZZ_LIBRARIES ${HARFBUZZ_LIBRARY})
  set(HARFBUZZ_INCLUDE_DIRS ${HARFBUZZ_INCLUDE_DIR})

  if(NOT TARGET HarfBuzz::HarfBuzz)
    add_library(HarfBuzz::HarfBuzz UNKNOWN IMPORTED)
    set_target_properties(HarfBuzz::HarfBuzz PROPERTIES
                                             IMPORTED_LOCATION "${HARFBUZZ_LIBRARY}"
                                             INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(HARFBUZZ_INCLUDE_DIR HARFBUZZ_LIBRARY)
