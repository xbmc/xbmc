#.rst:
# FindFreetype
# ------------
# Finds the FreeType library
#
# This will will define the following variables::
#
# FREETYPE_FOUND - system has FreeType
# FREETYPE_INCLUDE_DIRS - the FreeType include directory
# FREETYPE_LIBRARIES - the FreeType libraries
#
# and the following imported targets::
#
#   FreeType::FreeType   - The FreeType library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_FREETYPE freetype2 QUIET)
endif()

find_path(FREETYPE_INCLUDE_DIR NAMES freetype/freetype.h freetype.h
                               PATHS ${PC_FREETYPE_INCLUDEDIR}
                                     ${PC_FREETYPE_INCLUDE_DIRS})
find_library(FREETYPE_LIBRARY NAMES freetype freetype246MT
                              PATHS ${PC_FREETYPE_LIBDIR})

set(FREETYPE_VERSION ${PC_FREETYPE_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreeType
                                  REQUIRED_VARS FREETYPE_LIBRARY FREETYPE_INCLUDE_DIR
                                  VERSION_VAR FREETYPE_VERSION)

if(FREETYPE_FOUND)
  set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
  set(FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR})

  if(NOT TARGET FreeType::FreeType)
    add_library(FreeType::FreeType UNKNOWN IMPORTED)
    set_target_properties(FreeType::FreeType PROPERTIES
                                             IMPORTED_LOCATION "${FREETYPE_LIBRARY}"
                                             INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(FREETYPE_INCLUDE_DIR FREETYPE_LIBRARY)
