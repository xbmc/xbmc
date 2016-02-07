# - Try to find avahi
# Once done this will define
#
# AVAHI_FOUND - system has avahi
# AVAHI_INCLUDE_DIRS - the avahi include directory
# AVAHI_LIBRARIES - The avahi libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (AVAHI avahi-client)
  list(APPEND AVAHI_INCLUDE_DIRS ${AVAHI_INCLUDEDIR})
else()
  find_path(AVAHI_CLIENT_INCLUDE_DIRS avahi-client/client.h)
  find_path(AVAHI_COMMON_INCLUDE_DIRS avahi-common/defs.h)
  find_library(AVAHI_COMMON_LIBRARIES avahi-common)
  find_library(AVAHI_CLIENT_LIBRARIES avahi-common)
  set(AVAHI_INCLUDE_DIRS ${AVAHI_CLIENT_INCLUDE_DIRS}
                         ${AVAHI_COMMON_INCLUDE_DIRS})
  set(AVAHI_LIBRARIES ${AVAHI_CLIENT_LIBRARIES}
                      ${AVAHI_COMMON_LIBRARIES})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Avahi DEFAULT_MSG AVAHI_INCLUDE_DIRS AVAHI_LIBRARIES)

mark_as_advanced(AVAHI_INCLUDE_DIRS AVAHI_LIBRARIES)
list(APPEND AVAHI_DEFINITIONS -DHAVE_LIBAVAHI_COMMON=1 -DHAVE_LIBAVAHI_CLIENT=1)
