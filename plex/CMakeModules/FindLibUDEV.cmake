if(LIBUDEV_INCLUDE_DIR)
  # Already in cache, be silent
  set(LIBUDEV_FIND_QUIETLY TRUE)
endif(LIBUDEV_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_LIBUDEV libudev)
endif (PKG_CONFIG_FOUND)

Find_Path(LIBUDEV_INCLUDE_DIR
  NAMES libudev.h
  PATHS /usr/include usr/local/include 
  HINTS ${_LIBUDEV_INCLUDEDIR}
)

Find_Library(LIBUDEV_LIBRARY
  NAMES udev
  PATHS /lib /usr/lib usr/local/lib
  HINTS ${_LIBUDEV_LIBDIR}
)

include(FindPackageHandleStandardArgs)
message(${LIBUDEV_LIBRARY} ${LIBUDEV_INCLUDE_DIR})
find_package_handle_standard_args(LIBUDEV DEFAULT_MSG LIBUDEV_LIBRARY LIBUDEV_INCLUDE_DIR)

IF(LIBUDEV_LIBRARY AND LIBUDEV_INCLUDE_DIR)
  plex_get_soname(LIBUDEV_SONAME ${LIBUDEV_LIBRARY})
ENDIF()
