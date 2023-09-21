#.rst:
# FindDBUS
# -------
# Finds the DBUS library
#
# This will define the following target:
#
#   DBus::DBus   - The DBUS library

if(NOT TARGET DBus::DBus)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_DBUS dbus-1 QUIET)
  endif()

  find_path(DBUS_INCLUDE_DIR NAMES dbus/dbus.h
                             PATH_SUFFIXES dbus-1.0
                             HINTS ${PC_DBUS_INCLUDE_DIR})
  find_path(DBUS_ARCH_INCLUDE_DIR NAMES dbus/dbus-arch-deps.h
                                  PATH_SUFFIXES dbus-1.0/include
                                  HINTS ${PC_DBUS_LIBDIR}
                                  PATHS /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE})
  find_library(DBUS_LIBRARY NAMES dbus-1
                            HINTS ${PC_DBUS_LIBDIR})

  set(DBUS_VERSION ${PC_DBUS_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(DBus
                                    REQUIRED_VARS DBUS_LIBRARY DBUS_INCLUDE_DIR DBUS_ARCH_INCLUDE_DIR
                                    VERSION_VAR DBUS_VERSION)

  if(DBUS_FOUND)
    add_library(DBus::DBus UNKNOWN IMPORTED)
    set_target_properties(DBus::DBus PROPERTIES
                                   IMPORTED_LOCATION "${DBUS_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${DBUS_INCLUDE_DIR};${DBUS_ARCH_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAS_DBUS=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP DBus::DBus)
  endif()

  mark_as_advanced(DBUS_INCLUDE_DIR DBUS_LIBRARY)
endif()
