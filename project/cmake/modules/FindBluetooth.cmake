#.rst:
# FindBluetooth
# ---------
# Finds the Bluetooth library
#
# This will will define the following variables::
#
# BLUETOOTH_FOUND - system has Bluetooth
# BLUETOOTH_INCLUDE_DIRS - the Bluetooth include directory
# BLUETOOTH_LIBRARIES - the Bluetooth libraries
#
# and the following imported targets::
#
#   Bluetooth::Bluetooth   - The Bluetooth library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_BLUETOOTH bluetooth QUIET)
endif()

find_path(BLUETOOTH_INCLUDE_DIR NAMES bluetooth/bluetooth.h
                                PATHS ${PC_BLUETOOTH_INCLUDEDIR})
find_library(BLUETOOTH_LIBRARY NAMES bluetooth
                               PATHS ${PC_BLUETOOTH_LIBDIR})

set(BLUETOOTH_VERSION ${PC_BLUETOOTH_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bluetooth
                                  REQUIRED_VARS BLUETOOTH_LIBRARY BLUETOOTH_INCLUDE_DIR
                                  VERSION_VAR ${BLUETOOTH_VERSION})

if(BLUETOOTH_FOUND)
  set(BLUETOOTH_INCLUDE_DIRS ${BLUETOOTH_INCLUDE_DIR})
  set(BLUETOOTH_LIBRARIES ${BLUETOOTH_LIBRARY})

  if(NOT TARGET Bluetooth::Bluetooth)
    add_library(Bluetooth::Bluetooth UNKNOWN IMPORTED)
    set_target_properties(Bluetooth::Bluetooth PROPERTIES
                                               IMPORTED_LOCATION "${BLUETOOTH_LIBRARY}"
                                               INTERFACE_INCLUDE_DIRECTORIES "${BLUETOOTH_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(BLUETOOTH_INCLUDE_DIR BLUETOOTH_LIBRARY)
