#.rst:
# FindFreetype
# ------------
# Finds the FreeType library
#
# This will define the following target:
#
#   FreeType::FreeType   - The FreeType library

if(NOT TARGET FreeType::FreeType)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_FREETYPE freetype2 QUIET)
  endif()

  find_path(FREETYPE_INCLUDE_DIR NAMES freetype/freetype.h freetype.h
                                 PATHS ${PC_FREETYPE_INCLUDEDIR}
                                       ${PC_FREETYPE_INCLUDE_DIRS}
                                 PATH_SUFFIXES freetype2
                                 NO_CACHE)
  find_library(FREETYPE_LIBRARY NAMES freetype freetype246MT
                                PATHS ${PC_FREETYPE_LIBDIR}
                                NO_CACHE)

  set(FREETYPE_VERSION ${PC_FREETYPE_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(FreeType
                                    REQUIRED_VARS FREETYPE_LIBRARY FREETYPE_INCLUDE_DIR
                                    VERSION_VAR FREETYPE_VERSION)

  if(FREETYPE_FOUND)
    add_library(FreeType::FreeType UNKNOWN IMPORTED)
    set_target_properties(FreeType::FreeType PROPERTIES
                                             IMPORTED_LOCATION "${FREETYPE_LIBRARY}"
                                             INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIR}")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP FreeType::FreeType)
  endif()
endif()
