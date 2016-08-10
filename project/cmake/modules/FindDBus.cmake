#.rst:
# FindDBUS
# -------
# Finds the DBUS library
#
# This will will define the following variables::
#
# DBUS_FOUND - system has DBUS
# DBUS_INCLUDE_DIRS - the DBUS include directory
# DBUS_LIBRARIES - the DBUS libraries
# DBUS_DEFINITIONS - the DBUS definitions
#
# and the following imported targets::
#
#   DBus::DBus   - The DBUS library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_DBUS dbus-1 QUIET)
endif()

find_path(DBUS_INCLUDE_DIR NAMES dbus/dbus.h
                           PATH_SUFFIXES dbus-1.0
                           PATHS ${PC_DBUS_INCLUDE_DIR})
find_path(DBUS_ARCH_INCLUDE_DIR NAMES dbus/dbus-arch-deps.h
                                PATH_SUFFIXES dbus-1.0/include
                                PATHS ${PC_DBUS_LIBDIR}
                                      /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE})
find_library(DBUS_LIBRARY NAMES dbus-1
                          PATHS ${PC_DBUS_LIBDIR})

set(DBUS_VERSION ${PC_DBUS_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DBus
                                  REQUIRED_VARS DBUS_LIBRARY DBUS_INCLUDE_DIR DBUS_ARCH_INCLUDE_DIR
                                  VERSION_VAR DBUS_VERSION)

if(DBUS_FOUND)
  set(DBUS_LIBRARIES ${DBUS_LIBRARY})
  set(DBUS_INCLUDE_DIRS ${DBUS_INCLUDE_DIR} ${DBUS_ARCH_INCLUDE_DIR})
  set(DBUS_DEFINITIONS -DHAVE_DBUS=1)

  if(NOT TARGET DBus::DBus)
    add_library(DBus::DBus UNKNOWN IMPORTED)
    set_target_properties(DBus::DBus PROPERTIES
                                   IMPORTED_LOCATION "${DBUS_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${DBUS_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_DBUS=1)
  endif()
endif()

mark_as_advanced(DBUS_INCLUDE_DIR DBUS_LIBRARY)
