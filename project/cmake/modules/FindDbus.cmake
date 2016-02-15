# - Try to find dbus
# Once done this will define
#
# DBUS_FOUND - system has libdbus
# DBUS_INCLUDE_DIRS - the libdbus include directory
# DBUS_LIBRARIES - The libdbus libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (DBUS dbus-1)
endif()

if(DBUS_FOUND)
  find_path(DBUS_INCLUDE_DIRS dbus/dbus.h PATH_SUFFIXES dbus-1.0)
  find_library(DBUS_LIBRARIES dbus-1.0)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Dbus DEFAULT_MSG DBUS_INCLUDE_DIRS DBUS_LIBRARIES)

list(APPEND DBUS_DEFINITIONS -DHAVE_DBUS=1)
mark_as_advanced(DBUS_INCLUDE_DIRS DBUS_LIBRARIES DBUS_DEFINITIONS)
