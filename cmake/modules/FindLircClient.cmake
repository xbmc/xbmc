# FindLircClient
# -----------
# Finds the liblirc_client library
#
# This will define the following variables::
#
#  LIRCCLIENT_FOUND         - if false, do not try to link to lirc_client
#  LIRCCLIENT_INCLUDE_DIRS  - where to find lirc/lirc_client.h
#  LIRCCLIENT_LIBRARYS      - the library to link against
#  LIRCCLIENT_DEFINITIONS   - the lirc definitions

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_LIRC lirc QUIET)
endif()

find_path(LIRCCLIENT_INCLUDE_DIR lirc/lirc_client.h PATHS ${PC_LIRC_INCLUDEDIR})
find_library(LIRCCLIENT_LIBRARY lirc_client PATHS ${PC_LIRC_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LircClient
                                  REQUIRED_VARS LIRCCLIENT_LIBRARY LIRCCLIENT_INCLUDE_DIR)

if(LIRCCLIENT_FOUND)
  set(LIRCCLIENT_LIBRARIES ${LIRCCLIENT_LIBRARY})
  set(LIRCCLIENT_INCLUDE_DIRS ${LIRCCLIENT_INCLUDE_DIR})
  set(LIRCCLIENT_DEFINITIONS -DHAS_LIRC=1)

  if(NOT TARGET LIRCCLIENT::LIRCCLIENT)
    add_library(LIRCCLIENT::LIRCCLIENT UNKNOWN IMPORTED)
    set_target_properties(LIRCCLIENT::LIRCCLIENT PROPERTIES
                                     IMPORTED_LOCATION "${LIRCCLIENT_LIBRARYS}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${LIRCCLIENT_INCLUDE_DIRS}")
  endif()
endif()

mark_as_advanced(LIRCCLIENT_LIBRARY LIRCCLIENT_INCLUDE_DIR)