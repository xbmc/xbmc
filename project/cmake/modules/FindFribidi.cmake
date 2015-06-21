# - Try to find Fribidi
# Once done this will define
#
# FRIBIDI_FOUND - system has fribidi
# FRIBIDI_INCLUDE_DIRS - the fribidi include directory
# FRIBIDI_LIBRARIES - The fribidi libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (FRIBIDI fribidi)
else()
  find_path(FRIBIDI_INCLUDE_DIRS fribidi/fribidi.h)
  find_library(FRIBIDI_LIBRARIES NAMES fribidi libfribidi)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fribidi DEFAULT_MSG FRIBIDI_INCLUDE_DIRS FRIBIDI_LIBRARIES)

mark_as_advanced(FRIBIDI_INCLUDE_DIRS FRIBIDI_LIBRARIES)
