#.rst:
# FindBluetooth
# ---------
# Finds the Bluetooth library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Bluetooth   - The Bluetooth library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_BLUETOOTH bluez bluetooth ${SEARCH_QUIET})
  endif()

  find_path(BLUETOOTH_INCLUDE_DIR NAMES bluetooth/bluetooth.h
                                  HINTS ${PC_BLUETOOTH_INCLUDEDIR})
  find_library(BLUETOOTH_LIBRARY NAMES bluetooth libbluetooth
                                 HINTS ${PC_BLUETOOTH_LIBDIR})

  set(BLUETOOTH_VERSION ${PC_BLUETOOTH_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Bluetooth
                                    REQUIRED_VARS BLUETOOTH_LIBRARY BLUETOOTH_INCLUDE_DIR
                                    VERSION_VAR BLUETOOTH_VERSION)

  if(BLUETOOTH_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${BLUETOOTH_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${BLUETOOTH_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBBLUETOOTH)
  endif()
endif()
