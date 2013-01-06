if(RTMP_INCLUDE_DIR)
  # Already in cache, be silent
  set(RTMP_FIND_QUIETLY TRUE)
endif(RTMP_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_RTMP librtmp)
endif (PKG_CONFIG_FOUND)

Find_Path(RTMP_INCLUDE_DIR
  NAMES librtmp/rtmp.h
  PATHS /usr/include usr/local/include 
  HINTS ${_RTMP_INCLUDEDIR}
)

Find_Library(RTMP_LIBRARY
  NAMES rtmp
  PATHS /usr/lib usr/local/lib
  HINTS ${_RTMP_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RTMP DEFAULT_MSG RTMP_LIBRARY RTMP_INCLUDE_DIR)

IF(RTMP_LIBRARY AND RTMP_INCLUDE_DIR)
  plex_get_soname(RTMP_SONAME ${RTMP_LIBRARY})
ENDIF()
