if(LIBMAD_INCLUDE_DIR)
  # Already in cache, be silent
  set(LIBMAD_FIND_QUIETLY TRUE)
endif(LIBMAD_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_LIBMAD mad)
endif (PKG_CONFIG_FOUND)

Find_Path(LIBMAD_INCLUDE_DIR
  NAMES mad.h
  PATHS /usr/include usr/local/include 
  HINTS ${_LIBMAD_INCLUDEDIR}
)

Find_Library(LIBMAD_LIBRARY
  NAMES mad
  PATHS /usr/lib usr/local/lib
  HINTS ${_LIBMAD_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBMAD DEFAULT_MSG LIBMAD_LIBRARY)

IF(LIBMAD_LIBRARY AND LIBMAD_INCLUDE_DIR)
  plex_get_soname(MAD_SONAME ${LIBMAD_LIBRARY})
ENDIF()
