# - Try to find rtmp
# Once done this will define
#
# RTMP_FOUND - system has librtmp
# RTMP_INCLUDE_DIRS - the librtmp include directory
# RTMP_LIBRARIES - The librtmp libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (RTMP librtmp)
  list(APPEND RTMP_INCLUDE_DIRS ${RTMP_INCLUDEDIR})
else()
  find_path(RTMP_INCLUDE_DIRS librtmp/rtmp.h)
  find_library(RTMP_LIBRARIES rtmp)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RTMP DEFAULT_MSG RTMP_INCLUDE_DIRS RTMP_LIBRARIES)

list(APPEND RTMP_DEFINITIONS -DHAS_LIBRTMP=1)

mark_as_advanced(RTMP_INCLUDE_DIRS RTMP_LIBRARIES RTMP_DEFINITIONS)
