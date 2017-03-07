# FindFmt
# -------
# Finds the Fmt library
#
# This will define the following variables::
#
# FMT_FOUND - system has Fmt
# FMT_INCLUDE_DIRS - the Fmt include directory
# FMT_LIBRARIES - the Fmt libraries
#
# and the following imported targets::
#
#   Fmt::Fmt   - The Fmt library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_FMT libfmt QUIET)
endif()

find_path(FMT_INCLUDE_DIR NAMES fmt/format.h
                          PATHS ${PC_FMT_INCLUDEDIR})

find_library(FMT_LIBRARY_RELEASE NAMES fmt
                                PATHS ${PC_FMT_LIBDIR})
find_library(FMT_LIBRARY_DEBUG NAMES fmtd
                               PATHS ${PC_FMT_LIBDIR})

include(SelectLibraryConfigurations)
select_library_configurations(FMT)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fmt
                                  REQUIRED_VARS FMT_LIBRARY FMT_INCLUDE_DIR)

if(FMT_FOUND)
  set(FMT_LIBRARIES ${FMT_LIBRARY})
  set(FMT_INCLUDE_DIRS ${FMT_INCLUDE_DIR})

  if(NOT TARGET Fmt::Fmt)
    add_library(Fmt::Fmt UNKNOWN IMPORTED)
    set_target_properties(Fmt::Fmt PROPERTIES
                                     IMPORTED_LOCATION "${FMT_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(FMT_INCLUDE_DIR FMT_LIBRARY)
