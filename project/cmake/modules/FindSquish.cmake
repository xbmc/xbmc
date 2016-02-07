# - Try to find SQUISH
# Once done this will define
#
# SQUISH - system has libuuid
# SQUISH_INCLUDE_DIRS - the libuuid include directory
# SQUISH_LIBRARIES - The libuuid libraries

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules (SQUISH squish)
else()
  find_path(SQUISH_INCLUDE_DIRS squish.h)
  find_library(SQUISH_LIBRARIES squish)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQUISH DEFAULT_MSG SQUISH_INCLUDE_DIRS SQUISH_LIBRARIES)

mark_as_advanced(SQUISH_INCLUDE_DIRS SQUISH_LIBRARIES)
