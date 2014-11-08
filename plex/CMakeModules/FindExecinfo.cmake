# -*- cmake -*-

# - Find execinfo
# Find the execinfo includes and library
# The problem with this library is that it is built-in in the Linux glib, 
# while on systems like FreeBSD, it is installed separately and thus needs to be linked to.
# Therefore, we search for the header to see if the it's available in the first place.
# If it is available, we try to locate the library to figure out whether it is built-in or not.

if(EXECINFO_INCLUDE_DIR)
  # Already in cache, be silent
  set(EXECINFO_FIND_QUIETLY TRUE)
endif(EXECINFO_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_EXECINFO libexecinfo)
endif (PKG_CONFIG_FOUND)

Find_Path(EXECINFO_INCLUDE_DIR
  NAMES execinfo.h
  PATHS /usr/include usr/local/include
  HINTS ${_EXECINFO_INCLUDEDIR}
)

Find_Library(EXECINFO_LIBRARY
  NAMES execinfo
  PATHS /usr/lib usr/local/lib
  HINTS ${_EXECINFO_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EXECINFO DEFAULT_MSG EXECINFO_LIBRARY EXECINFO_INCLUDE_DIR)

IF(EXECINFO_LIBRARY AND EXECINFO_INCLUDE_DIR)
  plex_get_soname(EXECINFO_SONAME ${EXECINFO_LIBRARY})
ENDIF()
