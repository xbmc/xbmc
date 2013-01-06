if(PLIST_INCLUDE_DIR)
  # Already in cache, be silent
  set(PLIST_FIND_QUIETLY TRUE)
endif(PLIST_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_PLIST libplist)
endif (PKG_CONFIG_FOUND)

Find_Path(PLIST_INCLUDE_DIR
  NAMES plist/plist.h
  PATHS /usr/include usr/local/include 
  HINTS ${_PLIST_INCLUDEDIR}
)

Find_Library(PLIST_LIBRARY
  NAMES plist
  PATHS /usr/lib usr/local/lib
  HINTS ${_PLIST_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PLIST DEFAULT_MSG PLIST_LIBRARY PLIST_INCLUDE_DIR)

IF(PLIST_LIBRARY AND PLIST_INCLUDE_DIR)
  plex_get_soname(PLIST_SONAME ${PLIST_LIBRARY})
ENDIF()
