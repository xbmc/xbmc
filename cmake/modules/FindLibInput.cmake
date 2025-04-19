#.rst:
# FindLibinput
# --------
# Finds the libinput library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibInput   - The LibInput library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBINPUT libinput ${SEARCH_QUIET})
  endif()

  find_path(LIBINPUT_INCLUDE_DIR NAMES libinput.h
                                 HINTS ${PC_LIBINPUT_INCLUDEDIR})

  find_library(LIBINPUT_LIBRARY NAMES input
                                HINTS ${PC_LIBINPUT_LIBDIR})

  set(LIBINPUT_VERSION ${PC_LIBINPUT_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibInput
                                    REQUIRED_VARS LIBINPUT_LIBRARY LIBINPUT_INCLUDE_DIR
                                    VERSION_VAR LIBINPUT_VERSION)

  if(LIBINPUT_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${LIBINPUT_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBINPUT_INCLUDE_DIR}")
  else()
    if(LibInput_FIND_REQUIRED)
      message(FATAL_ERROR "Libinput libraries were not found.")
    endif()
  endif()
endif()
