#.rst:
# FindLibUSB
# ----------
# Finds the USB library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibUSB   - The USB library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig)

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBUSB libusb QUIET)
  endif()

  find_path(LIBUSB_INCLUDE_DIR usb.h
                               HINTS ${PC_LIBUSB_INCLUDEDIR})
  find_library(LIBUSB_LIBRARY NAMES usb
                              HINTS ${PC_LIBUSB_INCLUDEDIR})
  set(LIBUSB_VERSION ${PC_LIBUSB_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibUSB
                                    REQUIRED_VARS LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR
                                    VERSION_VAR LIBUSB_VERSION)

  if(LIBUSB_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${LIBUSB_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBUSB)
  endif()
endif()
