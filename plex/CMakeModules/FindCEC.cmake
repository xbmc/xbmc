if(CEC_INCLUDE_DIR)
  # Already in cache, be silent
  set(CEC_FIND_QUIETLY TRUE)
endif(CEC_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_CEC libcec>=2.0)
endif (PKG_CONFIG_FOUND)

Find_Path(CEC_INCLUDE_DIR
  NAMES libcec/cec.h
  PATHS /usr/include usr/local/include 
  HINTS ${_CEC_INCLUDEDIR}
)

Find_Library(CEC_LIBRARY
  NAMES cec
  PATHS /usr/lib usr/local/lib
  HINTS ${_CEC_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CEC DEFAULT_MSG CEC_LIBRARY CEC_INCLUDE_DIR)

IF(CEC_LIBRARY AND CEC_INCLUDE_DIR)
  plex_get_soname(LIBCEC_SONAME ${CEC_LIBRARY})
ENDIF()
