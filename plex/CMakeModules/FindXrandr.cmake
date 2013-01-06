if(XRANDR_INCLUDE_DIR)
  # Already in cache, be silent
  set(XRANDR_FIND_QUIETLY TRUE)
endif(XRANDR_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_XRANDR xrandr)
endif (PKG_CONFIG_FOUND)

Find_Library(XRANDR_LIBRARY
  NAMES Xrandr
  PATHS /usr/lib usr/local/lib
  HINTS ${_XRANDR_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XRANDR DEFAULT_MSG XRANDR_LIBRARY)

IF(XRANDR_LIBRARY)
  SET(HAS_XRANDR 1)
ENDIF()
