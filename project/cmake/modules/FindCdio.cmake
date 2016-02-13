# - Try to find cdio
# Once done this will define
#
# CDIO_FOUND - system has libcdio
# CDIO_INCLUDE_DIRS - the libcdio include directory
# CDIO_LIBRARIES - The libcdio libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (CDIO libcdio libiso9660)
  list(APPEND CDIO_INCLUDE_DIRS ${CDIO_libcdio_INCLUDEDIR} ${CDIO_libiso9660_INCLUDEDIR})
endif()
if(NOT CDIO_FOUND)
  find_path(CDIO_INCLUDE_DIRS cdio/cdio.h)
  find_library(MODPLUG_LIBRARIES NAMES cdio)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cdio DEFAULT_MSG CDIO_INCLUDE_DIRS CDIO_LIBRARIES)

mark_as_advanced(CDIO_INCLUDE_DIRS CDIO_LIBRARIES)
