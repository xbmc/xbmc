#.rst:
# FindAvahi
# ---------
# Finds the avahi library
#
# This will define the following target:
#
#   Avahi::Avahi - The avahi client library
#   Avahi::AvahiCommon - The avahi common library

if(NOT TARGET Avahi::Avahi)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_AVAHI avahi-client QUIET)
  endif()

  find_path(AVAHI_CLIENT_INCLUDE_DIR NAMES avahi-client/client.h
                                     HINTS ${DEPENDS_PATH}/include ${PC_AVAHI_INCLUDEDIR}
                                     ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                     NO_CACHE)
  find_path(AVAHI_COMMON_INCLUDE_DIR NAMES avahi-common/defs.h
                                     HINTS ${DEPENDS_PATH}/include ${PC_AVAHI_INCLUDEDIR}
                                     ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                     NO_CACHE)
  find_library(AVAHI_CLIENT_LIBRARY NAMES avahi-client
                                    HINTS ${DEPENDS_PATH}/lib ${PC_AVAHI_LIBDIR}
                                    ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                    NO_CACHE)
  find_library(AVAHI_COMMON_LIBRARY NAMES avahi-common
                                    HINTS ${DEPENDS_PATH}/lib ${PC_AVAHI_LIBDIR}
                                    ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                    NO_CACHE)

  set(AVAHI_VERSION ${PC_AVAHI_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Avahi
                                    REQUIRED_VARS AVAHI_CLIENT_LIBRARY AVAHI_COMMON_LIBRARY
                                                  AVAHI_CLIENT_INCLUDE_DIR AVAHI_COMMON_INCLUDE_DIR
                                    VERSION_VAR AVAHI_VERSION)

  if(AVAHI_FOUND)
    add_library(Avahi::AvahiCommon UNKNOWN IMPORTED)
    set_target_properties(Avahi::AvahiCommon PROPERTIES
                                             IMPORTED_LOCATION "${AVAHI_COMMON_LIBRARY}"
                                             INTERFACE_INCLUDE_DIRECTORIES "${AVAHI_COMMON_INCLUDE_DIR}"
                                             INTERFACE_COMPILE_DEFINITIONS "HAS_AVAHI=1;HAS_ZEROCONF=1")
    add_library(Avahi::Avahi UNKNOWN IMPORTED)
    set_target_properties(Avahi::Avahi PROPERTIES
                                       IMPORTED_LOCATION "${AVAHI_CLIENT_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${AVAHI_CLIENT_INCLUDE_DIR}"
                                       INTERFACE_COMPILE_DEFINITIONS "HAS_AVAHI=1;HAS_ZEROCONF=1"
                                       INTERFACE_LINK_LIBRARIES Avahi::AvahiCommon)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Avahi::Avahi)
  endif()
endif()
