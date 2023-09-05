#.rst:
# FindHarfbuzz
# ------------
# Finds the HarfBuzz library
#
# This will define the following target:
#
#   HarfBuzz::HarfBuzz   - The HarfBuzz library

if(NOT TARGET HarfBuzz::HarfBuzz)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_HARFBUZZ harfbuzz QUIET)
  endif()

  find_path(HARFBUZZ_INCLUDE_DIR NAMES harfbuzz/hb-ft.h hb-ft.h
                                 HINTS ${PC_HARFBUZZ_INCLUDEDIR}
                                       ${PC_HARFBUZZ_INCLUDE_DIRS}
                                 PATH_SUFFIXES harfbuzz
                                 NO_CACHE)
  find_library(HARFBUZZ_LIBRARY NAMES harfbuzz
                                HINTS ${PC_HARFBUZZ_LIBDIR}
                                NO_CACHE)

  set(HARFBUZZ_VERSION ${PC_HARFBUZZ_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(HarfBuzz
                                    REQUIRED_VARS HARFBUZZ_LIBRARY HARFBUZZ_INCLUDE_DIR
                                    VERSION_VAR HARFBUZZ_VERSION)

  if(HARFBUZZ_FOUND)
    add_library(HarfBuzz::HarfBuzz UNKNOWN IMPORTED)
    set_target_properties(HarfBuzz::HarfBuzz PROPERTIES
                                             IMPORTED_LOCATION "${HARFBUZZ_LIBRARY}"
                                             INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIR}")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP HarfBuzz::HarfBuzz)
  endif()
endif()
