if(CDIO_INCLUDE_DIR)
  # Already in cache, be silent
  set(CDIO_FIND_QUIETLY TRUE)
endif(CDIO_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_CDIO libcdio)
endif (PKG_CONFIG_FOUND)

Find_Path(CDIO_INCLUDE_DIR
  NAMES cdio/cdio.h
  PATHS /usr/include usr/local/include 
  HINTS ${_CDIO_INCLUDEDIR}
)

Find_Library(CDIO_LIBRARY
  NAMES cdio
  PATHS /usr/lib usr/local/lib
  HINTS ${_CDIO_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CDIO DEFAULT_MSG CDIO_LIBRARY CDIO_INCLUDE_DIR)

IF(CDIO_LIBRARY AND CDIO_INCLUDE_DIR)
  plex_get_soname(CDIO_SONAME ${CDIO_LIBRARY})
ENDIF()
