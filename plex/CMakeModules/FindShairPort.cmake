if(SHAIRPORT_INCLUDE_DIR)
  # Already in cache, be silent
  set(SHAIRPORT_FIND_QUIETLY TRUE)
endif(SHAIRPORT_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_SHAIRPORT libshairport)
endif (PKG_CONFIG_FOUND)

Find_Path(SHAIRPORT_INCLUDE_DIR
  NAMES shairport/shairport.h
  PATHS /usr/include usr/local/include 
  HINTS ${_SHAIRPORT_INCLUDEDIR}
)

Find_Library(SHAIRPORT_LIBRARY
  NAMES shairport
  PATHS /usr/lib usr/local/lib
  HINTS ${_SHAIRPORT_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SHAIRPORT DEFAULT_MSG SHAIRPORT_LIBRARY SHAIRPORT_INCLUDE_DIR)

IF(SHAIRPORT_LIBRARY AND SHAIRPORT_INCLUDE_DIR)
  plex_get_soname(SHAIRPORT_SONAME ${SHAIRPORT_LIBRARY})
ENDIF()
