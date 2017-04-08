#.rst:
# FindAvahi
# ---------
# Finds the avahi library
#
# This will will define the following variables::
#
# AVAHI_FOUND - system has avahi
# AVAHI_INCLUDE_DIRS - the avahi include directory
# AVAHI_LIBRARIES - the avahi libraries
# AVAHI_DEFINITIONS - the avahi definitions
#
# and the following imported targets::
#
#   Avahi::Avahi   - The avahi library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_AVAHI avahi-client QUIET)
endif()

find_path(AVAHI_CLIENT_INCLUDE_DIR NAMES avahi-client/client.h
                                   PATHS ${PC_AVAHI_INCLUDEDIR})
find_path(AVAHI_COMMON_INCLUDE_DIR NAMES avahi-common/defs.h
                                   PATHS ${PC_AVAHI_INCLUDEDIR})
find_library(AVAHI_CLIENT_LIBRARY NAMES avahi-client
                                  PATHS ${PC_AVAHI_LIBDIR})
find_library(AVAHI_COMMON_LIBRARY NAMES avahi-common
                                  PATHS ${PC_AVAHI_LIBDIR})

set(AVAHI_VERSION ${PC_AVAHI_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Avahi
                                  REQUIRED_VARS AVAHI_CLIENT_LIBRARY AVAHI_COMMON_LIBRARY
                                                AVAHI_CLIENT_INCLUDE_DIR AVAHI_COMMON_INCLUDE_DIR
                                  VERSION_VAR AVAHI_VERSION)

if(AVAHI_FOUND)
  set(AVAHI_INCLUDE_DIRS ${AVAHI_CLIENT_INCLUDE_DIR}
                         ${AVAHI_COMMON_INCLUDE_DIR})
  set(AVAHI_LIBRARIES ${AVAHI_CLIENT_LIBRARY}
                      ${AVAHI_COMMON_LIBRARY})
  set(AVAHI_DEFINITIONS -DHAVE_LIBAVAHI_CLIENT=1 -DHAVE_LIBAVAHI_COMMON=1)

  if(NOT TARGET Avahi::Avahi)
    add_library(Avahi::Avahi UNKNOWN IMPORTED)
    set_target_properties(Avahi::Avahi PROPERTIES
                                       IMPORTED_LOCATION "${AVAHI_CLIENT_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${AVAHI_CLIENT_INCLUDE_DIR}"
                                       INTERFACE_COMPILE_DEFINITIONS HAVE_LIBAVAHI_CLIENT=1)
  endif()
  if(NOT TARGET Avahi::AvahiCommon)
    add_library(Avahi::AvahiCommon UNKNOWN IMPORTED)
    set_target_properties(Avahi::AvahiCommon PROPERTIES
                                             IMPORTED_LOCATION "${AVAHI_COMMON_LIBRARY}"
                                             INTERFACE_INCLUDE_DIRECTORIES "${AVAHI_COMMON_INCLUDE_DIR}"
                                             INTERFACE_COMPILE_DEFINITIONS HAVE_LIBAVAHI_COMMON=1
                                             INTERFACE_LINK_LIBRARIES Avahi::Avahi)
  endif()
endif()

mark_as_advanced(AVAHI_CLIENT_INCLUDE_DIR AVAHI_COMMON_INCLUDE_DIR
                 AVAHI_CLIENT_LIBRARY AVAHI_COMMON_LIBRARY)
