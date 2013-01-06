if(ALSA_INCLUDE_DIR)
  # Already in cache, be silent
  set(ALSA_FIND_QUIETLY TRUE)
endif(ALSA_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_ALSA alsa)
endif (PKG_CONFIG_FOUND)

Find_Path(ALSA_INCLUDE_DIR
  NAMES alsa/version.h
  PATHS /usr/include usr/local/include 
  HINTS ${_ALSA_INCLUDEDIR}
)

Find_Library(ALSA_LIBRARY
  NAMES asound
  PATHS /usr/lib usr/local/lib
  HINTS ${_ALSA_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ALSA DEFAULT_MSG ALSA_LIBRARY ALSA_INCLUDE_DIR)

IF(ALSA_LIBRARY AND ALSA_INCLUDE_DIR)
  set(HAS_ALSA 1)
ENDIF()
