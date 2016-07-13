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

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_BLUETOOTH bluetooth QUIET)
endif()

find_path(BLUETOOTH_INCLUDE_DIR NAMES bluetooth/bluetooth.h
                                PATHS ${PC_BLUETOOTH_INCLUDEDIR})
find_library(BLUETOOTH_LIBRARY NAMES bluetooth
                                PATHS ${PC_BLUETOOTH_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BLUETOOTH
                                  REQUIRED_VARS BLUETOOTH_LIBRARY BLUETOOTH_INCLUDE_DIR)

if(BLUETOOTH_FOUND)
  set(BLUETOOTH_INCLUDE_DIRS ${BLUETOOTH_INCLUDE_DIR})
  set(BLUETOOTH_LIBRARIES ${BLUETOOTH_LIBRARY})
endif()

mark_as_advanced(BLUETOOTH_INCLUDE_DIR BLUETOOTH_LIBRARY)
