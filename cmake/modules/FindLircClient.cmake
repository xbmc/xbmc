# FindLircClient
# -----------
# Finds the liblirc_client library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LircClient - The lirc library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
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
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${LIRCCLIENT_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIRCCLIENT_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAS_LIRC)
  endif()
endif()
