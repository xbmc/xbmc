#.rst:
# FindLCMS2
# -----------
# Finds the LCMS Color Management library
#
# This will will define the following variables::
#
# LCMS2_FOUND - system has LCMS Color Management
# LCMS2_INCLUDE_DIRS - the LCMS Color Management include directory
# LCMS2_LIBRARIES - the LCMS Color Management libraries
# LCMS2_DEFINITIONS - the LCMS Color Management definitions
#
# and the following imported targets::
#
#   LCMS2::LCMS2   - The LCMS Color Management library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_LCMS2 lcms2 QUIET)
endif()

find_path(LCMS2_INCLUDE_DIR NAMES lcms2.h
                            PATHS ${PC_LCMS2_INCLUDEDIR})
find_library(LCMS2_LIBRARY NAMES lcms2 liblcms2
                           PATHS ${PC_LCMS2_LIBDIR})

set(LCMS2_VERSION ${PC_LCMS2_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LCMS2
                                  REQUIRED_VARS LCMS2_LIBRARY LCMS2_INCLUDE_DIR
                                  VERSION_VAR LCMS2_VERSION)

if(LCMS2_FOUND)
  set(LCMS2_LIBRARIES ${LCMS2_LIBRARY})
  set(LCMS2_INCLUDE_DIRS ${LCMS2_INCLUDE_DIR})
  set(LCMS2_DEFINITIONS -DHAVE_LCMS2=1)

  if(NOT TARGET LCMS2::LCMS2)
    add_library(LCMS2::LCMS2 UNKNOWN IMPORTED)
    set_target_properties(LCMS2::LCMS2 PROPERTIES
                                       IMPORTED_LOCATION "${LCMS2_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${LCMS2_INCLUDE_DIR}"
                                       INTERFACE_COMPILE_DEFINITIONS HAVE_LCMS2=1)
  endif()
endif()

mark_as_advanced(LCMS2_INCLUDE_DIR LCMS2_LIBRARY)

