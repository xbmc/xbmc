if(ASS_INCLUDE_DIR)
  # Already in cache, be silent
  set(ASS_FIND_QUIETLY TRUE)
endif(ASS_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_ASS libass)
endif (PKG_CONFIG_FOUND)

Find_Path(ASS_INCLUDE_DIR
  NAMES ass/ass.h
  PATHS /usr/include usr/local/include 
  HINTS ${_ASS_INCLUDEDIR}
)

Find_Library(ASS_LIBRARY
  NAMES ass
  PATHS /usr/lib usr/local/lib
  HINTS ${_ASS_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ASS DEFAULT_MSG ASS_LIBRARY ASS_INCLUDE_DIR)

IF(ASS_LIBRARY AND ASS_INCLUDE_DIR)
  plex_get_soname(ASS_SONAME ${ASS_LIBRARY})
ENDIF()
