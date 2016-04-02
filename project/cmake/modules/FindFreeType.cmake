# - Try to find freetype
# Once done this will define
#
# FREETYPE_FOUND - system has freetype
# FREETYPE_INCLUDE_DIRS - the freetype include directory
# FREETYPE_LIBRARIES - The freetype libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (FREETYPE freetype2)
else()
  find_path(FREETYPE_INCLUDE_DIRS freetype/freetype.h)
  find_library(FREETYPE_LIBRARIES NAMES freetype freetype246MT)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreeType DEFAULT_MSG FREETYPE_INCLUDE_DIRS FREETYPE_LIBRARIES)

mark_as_advanced(FREETYPE_INCLUDE_DIRS FREETYPE_LIBRARIES)
