if(MPEG2_INCLUDE_DIR)
  # Already in cache, be silent
  set(MPEG2_FIND_QUIETLY TRUE)
endif(MPEG2_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_MPEG2 libmpeg2)
endif (PKG_CONFIG_FOUND)

Find_Path(MPEG2_INCLUDE_DIR
  NAMES mpeg2dec/mpeg2.h
  PATHS /usr/include usr/local/include 
  HINTS ${_MPEG2_INCLUDEDIR}
)

Find_Library(MPEG2_LIBRARY
  NAMES mpeg2
  PATHS /usr/lib usr/local/lib
  HINTS ${_MPEG2_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MPEG2 DEFAULT_MSG MPEG2_LIBRARY MPEG2_INCLUDE_DIR)

IF(MPEG2_LIBRARY AND MPEG2_INCLUDE_DIR)
  SET(MPEG2_FOUND TRUE CACHE STRING "do we have mpeg2")
  plex_get_soname(MPEG2_SONAME ${MPEG2_LIBRARY})
ENDIF()
