# FindLircClient
# -----------
# Finds the liblirc_client library
#
# This will define the following target:
#
#   LIRCCLIENT::LIRCCLIENT - The lirc library

if(NOT TARGET LIRCCLIENT::LIRCCLIENT)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIRC lirc QUIET)
  endif()

  find_path(LIRCCLIENT_INCLUDE_DIR lirc/lirc_client.h
                                   HINTS ${PC_LIRC_INCLUDEDIR})
  find_library(LIRCCLIENT_LIBRARY lirc_client
                                  HINTS ${PC_LIRC_LIBDIR})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LircClient
                                    REQUIRED_VARS LIRCCLIENT_LIBRARY LIRCCLIENT_INCLUDE_DIR)

  if(LIRCCLIENT_FOUND)
    add_library(LIRCCLIENT::LIRCCLIENT UNKNOWN IMPORTED)
    set_target_properties(LIRCCLIENT::LIRCCLIENT PROPERTIES
                                                 IMPORTED_LOCATION "${LIRCCLIENT_LIBRARY}"
                                                 INTERFACE_INCLUDE_DIRECTORIES "${LIRCCLIENT_INCLUDE_DIR}"
                                                 INTERFACE_COMPILE_DEFINITIONS "HAS_LIRC=1")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LIRCCLIENT::LIRCCLIENT)
  endif()

  mark_as_advanced(LIRCCLIENT_LIBRARY LIRCCLIENT_INCLUDE_DIR)
endif()
