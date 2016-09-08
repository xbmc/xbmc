#.rst:
# FindLibUSB
# ----------
# Finds the USB library
#
# This will will define the following variables::
#
# LIBUSB_FOUND - system has LibUSB
# LIBUSB_INCLUDE_DIRS - the USB include directory
# LIBUSB_LIBRARIES - the USB libraries
#
# and the following imported targets::
#
#   LibUSB::LibUSB   - The USB library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_LIBUSB libusb QUIET)
endif()

find_path(LIBUSB_INCLUDE_DIR usb.h
                             PATHS ${PC_LIBUSB_INCLUDEDIR})
find_library(LIBUSB_LIBRARY NAMES usb
                            PATHS ${PC_LIBUSB_INCLUDEDIR})
set(LIBUSB_VERSION ${PC_LIBUSB_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBUSB
                                  REQUIRED_VARS LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR
                                  VERSION_VAR LIBUSB_VERSION)

if(LIBUSB_FOUND)
  set(LIBUSB_INCLUDE_DIRS ${LIBUSB_INCLUDE_DIR})
  set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})
  set(LIBUSB_DEFINITIONS -DUSE_LIBUSB=1)

  if(NOT TARGET LibUSB::LibUSB)
    add_library(LibUSB::LibUSB UNKNOWN IMPORTED)
    set_target_properties(LibUSB::LibUSB PROPERTIES
                                         IMPORTED_LOCATION "${LIBUSB_LIBRARY}"
                                         INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}"
                                         INTERFACE_COMPILE_DEFINITIONS USE_LIBUSB=1)
  endif()
endif()

mark_as_advanced(USB_INCLUDE_DIR USB_LIBRARY)
