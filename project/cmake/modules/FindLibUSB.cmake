# - Try to find libusb
# Once done this will define
#
# USB_FOUND - system has libusb
# USB_INCLUDE_DIRS - the libusb include directory
# USB_LIBRARIES - The libusb libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (usb libusb)
  list(APPEND USB_INCLUDE_DIRS ${USB_INCLUDEDIR})
else()
  find_path(USB_INCLUDE_DIRS usb.h)
  find_library(USB_LIBRARIES usb)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibUSB DEFAULT_MSG USB_INCLUDE_DIRS USB_LIBRARIES)

list(APPEND USB_DEFINITIONS -DUSE_LIBUSB=1)
mark_as_advanced(USB_INCLUDE_DIRS USB_LIBRARIES USB_DEFINITIONS)
