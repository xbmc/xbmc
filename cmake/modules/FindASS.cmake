#.rst:
# FindASS
# -------
# Finds the ASS library
#
# This will will define the following variables::
#
# ASS_FOUND - system has ASS
# ASS_INCLUDE_DIRS - the ASS include directory
# ASS_LIBRARIES - the ASS libraries
#
# and the following imported targets::
#
#   ASS::ASS   - The ASS library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_ASS libass QUIET)
endif()

find_path(ASS_INCLUDE_DIR NAMES ass/ass.h
                          PATHS ${PC_ASS_INCLUDEDIR})
find_library(ASS_LIBRARY NAMES ass libass
                         PATHS ${PC_ASS_LIBDIR})

set(ASS_VERSION ${PC_ASS_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ASS
                                  REQUIRED_VARS ASS_LIBRARY ASS_INCLUDE_DIR
                                  VERSION_VAR ASS_VERSION)

if(ASS_FOUND)
  set(ASS_LIBRARIES ${ASS_LIBRARY})
  set(ASS_INCLUDE_DIRS ${ASS_INCLUDE_DIR})

  if(NOT TARGET ASS::ASS)
    add_library(ASS::ASS UNKNOWN IMPORTED)
    set_target_properties(ASS::ASS PROPERTIES
                                   IMPORTED_LOCATION "${ASS_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${ASS_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(ASS_INCLUDE_DIR ASS_LIBRARY)
