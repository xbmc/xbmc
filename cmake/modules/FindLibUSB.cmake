#.rst:
# FindLibUSB
# ----------
# Finds the USB library
#
# This will define the following target:
#
#   LibUSB::LibUSB   - The USB library

if(NOT TARGET LibUSB::LibUSB)
  find_package(PkgConfig)

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBUSB libusb QUIET)
  endif()

  find_path(LIBUSB_INCLUDE_DIR usb.h
                               HINTS ${PC_LIBUSB_INCLUDEDIR}
                               NO_CACHE)
  find_library(LIBUSB_LIBRARY NAMES usb
                              HINTS ${PC_LIBUSB_INCLUDEDIR}
                              NO_CACHE)
  set(LIBUSB_VERSION ${PC_LIBUSB_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibUSB
                                    REQUIRED_VARS LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR
                                    VERSION_VAR LIBUSB_VERSION)

  if(LIBUSB_FOUND)
    add_library(LibUSB::LibUSB UNKNOWN IMPORTED)
    set_target_properties(LibUSB::LibUSB PROPERTIES
                                         IMPORTED_LOCATION "${LIBUSB_LIBRARY}"
                                         INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}"
                                         INTERFACE_COMPILE_DEFINITIONS USE_LIBUSB=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LibUSB::LibUSB)
  endif()
endif()
