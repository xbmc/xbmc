#.rst:
# FindUDEV
# -------
# Finds the UDEV library
#
# This will define the following target:
#
#   UDEV::UDEV   - The UDEV library

if(NOT TARGET UDEV::UDEV)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_UDEV libudev QUIET)
  endif()

  find_path(UDEV_INCLUDE_DIR NAMES libudev.h
                             HINTS ${PC_UDEV_INCLUDEDIR}
                             NO_CACHE)
  find_library(UDEV_LIBRARY NAMES udev
                            HINTS ${PC_UDEV_LIBDIR}
                            NO_CACHE)

  set(UDEV_VERSION ${PC_UDEV_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(UDEV
                                    REQUIRED_VARS UDEV_LIBRARY UDEV_INCLUDE_DIR
                                    VERSION_VAR UDEV_VERSION)

  if(UDEV_FOUND)
    add_library(UDEV::UDEV UNKNOWN IMPORTED)
    set_target_properties(UDEV::UDEV PROPERTIES
                                   IMPORTED_LOCATION "${UDEV_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${UDEV_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_LIBUDEV=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP UDEV::UDEV)
  endif()
endif()
