if(LIBUSB_INCLUDE_DIR)
  # Already in cache, be silent
  set(LIBUSB_FIND_QUIETLY TRUE)
endif(LIBUSB_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_LIBUSB libusb)
endif (PKG_CONFIG_FOUND)

Find_Path(LIBUSB_INCLUDE_DIR
  NAMES usb.h
  PATHS /usr/include usr/local/include 
  HINTS ${_LIBUSB_INCLUDEDIR}
)

Find_Library(LIBUSB_LIBRARY
  NAMES usb usb-1.0
  PATHS /lib/x86_64-linux-gnu /lib /usr/lib usr/local/lib
  HINTS ${_LIBUSB_LIBDIR}
)

include(FindPackageHandleStandardArgs)
message(${LIBUSB_LIBRARY} ${LIBUSB_INCLUDE_DIR})
find_package_handle_standard_args(LIBUSB DEFAULT_MSG LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)

IF(LIBUSB_LIBRARY AND LIBUSB_INCLUDE_DIR)
  plex_get_soname(LIBUSB_SONAME ${LIBUSB_LIBRARY})
ENDIF()
