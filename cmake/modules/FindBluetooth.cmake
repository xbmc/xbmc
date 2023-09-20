#.rst:
# FindBluetooth
# ---------
# Finds the Bluetooth library
#
# This will define the following target:
#
#   Bluetooth::Bluetooth   - The Bluetooth library

if(NOT TARGET Bluetooth::Bluetooth)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_BLUETOOTH bluez bluetooth QUIET)
  endif()

  find_path(BLUETOOTH_INCLUDE_DIR NAMES bluetooth/bluetooth.h
                                  HINTS ${PC_BLUETOOTH_INCLUDEDIR}
                                  NO_CACHE)
  find_library(BLUETOOTH_LIBRARY NAMES bluetooth libbluetooth
                                 HINTS ${PC_BLUETOOTH_LIBDIR}
                                 NO_CACHE)

  set(BLUETOOTH_VERSION ${PC_BLUETOOTH_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Bluetooth
                                    REQUIRED_VARS BLUETOOTH_LIBRARY BLUETOOTH_INCLUDE_DIR
                                    VERSION_VAR BLUETOOTH_VERSION)

  if(BLUETOOTH_FOUND)
    add_library(Bluetooth::Bluetooth UNKNOWN IMPORTED)
    set_target_properties(Bluetooth::Bluetooth PROPERTIES
                                               IMPORTED_LOCATION "${BLUETOOTH_LIBRARY}"
                                               INTERFACE_INCLUDE_DIRECTORIES "${BLUETOOTH_INCLUDE_DIR}"
                                               INTERFACE_COMPILE_DEFINITIONS HAVE_LIBBLUETOOTH=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Bluetooth::Bluetooth)
  endif()
endif()
