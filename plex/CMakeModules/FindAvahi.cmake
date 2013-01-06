if(AVAHI_INCLUDE_DIR)
  # Already in cache, be silent
  set(AVAHI_FIND_QUIETLY TRUE)
endif(AVAHI_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_AVAHI avahi-client)
endif (PKG_CONFIG_FOUND)

Find_Path(AVAHI_INCLUDE_DIR
  NAMES avahi-client/client.h
  PATHS /usr/include usr/local/include 
  HINTS ${_AVAHI_INCLUDEDIR}
)

Find_Library(AVAHICLIENT_LIBRARY
  NAMES avahi-client
  PATHS /usr/lib usr/local/lib
  HINTS ${_AVAHI_LIBDIR}
)

Find_Library(AVAHICOMMON_LIBRARY
  NAMES avahi-common
  PATHS /usr/lib usr/local/lib
  HINTS ${_AVAHI_LIBDIR}
)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVAHI DEFAULT_MSG AVAHICLIENT_LIBRARY AVAHI_INCLUDE_DIR AVAHICOMMON_LIBRARY)

IF(AVAHICLIENT_LIBRARY AND AVAHI_INCLUDE_DIR AND AVAHICOMMON_LIBRARY)
  SET(HAVE_LIBAVAHI_CLIENT 1)
  SET(HAVE_LIBAVAHI_COMMON 1)
  SET(AVAHI_FOUND TRUE CACHE STRING "do we have libavahi")
  SET(AVAHI_LIBRARIES ${AVAHICLIENT_LIBRARY} ${AVAHICOMMON_LIBRARY})
ENDIF()
