# - Try to find the low-level D-Bus library
# Once done this will define
#
#  DBUS_FOUND - system has D-Bus
#  DBUS_INCLUDE_DIR - the D-Bus include directory
#  DBUS_ARCH_INCLUDE_DIR - the D-Bus architecture-specific include directory
#  DBUS_LIBRARIES - the libraries needed to use D-Bus

# Copyright (c) 2008, Kevin Kofler, <kevin.kofler@chello.at>
# modeled after FindLibArt.cmake:
# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (DBUS_INCLUDE_DIR AND DBUS_ARCH_INCLUDE_DIR AND DBUS_LIBRARIES)

  # in cache already
  SET(DBUS_FOUND TRUE)

else (DBUS_INCLUDE_DIR AND DBUS_ARCH_INCLUDE_DIR AND DBUS_LIBRARIES)

  IF (NOT WIN32)
    FIND_PACKAGE(PkgConfig)
    IF (PKG_CONFIG_FOUND)
      # use pkg-config to get the directories and then use these values
      # in the FIND_PATH() and FIND_LIBRARY() calls
      pkg_check_modules(_DBUS_PC dbus-1)
    ENDIF (PKG_CONFIG_FOUND)
  ENDIF (NOT WIN32)

  FIND_PATH(DBUS_INCLUDE_DIR dbus/dbus.h
    ${_DBUS_PC_INCLUDE_DIRS}
    /usr/include
    /usr/include/dbus-1.0
    /usr/local/include
  )

  FIND_PATH(DBUS_ARCH_INCLUDE_DIR dbus/dbus-arch-deps.h
    ${_DBUS_PC_INCLUDE_DIRS}
    /usr/lib${LIB_SUFFIX}/include
    /usr/lib${LIB_SUFFIX}/dbus-1.0/include
    /usr/lib64/include
    /usr/lib64/dbus-1.0/include
    /usr/lib/include
    /usr/lib/dbus-1.0/include
  )

  FIND_LIBRARY(DBUS_LIBRARIES NAMES dbus-1 dbus
    PATHS
     ${_DBUS_PC_LIBDIR}
  )


  if (DBUS_INCLUDE_DIR AND DBUS_ARCH_INCLUDE_DIR AND DBUS_LIBRARIES)
     set(DBUS_FOUND TRUE)
  endif (DBUS_INCLUDE_DIR AND DBUS_ARCH_INCLUDE_DIR AND DBUS_LIBRARIES)


  if (DBUS_FOUND)
     if (NOT DBus_FIND_QUIETLY)
        message(STATUS "Found D-Bus: ${DBUS_LIBRARIES}")
     endif (NOT DBus_FIND_QUIETLY)
  else (DBUS_FOUND)
     if (DBus_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find D-Bus")
     endif (DBus_FIND_REQUIRED)
  endif (DBUS_FOUND)

  MARK_AS_ADVANCED(DBUS_INCLUDE_DIR DBUS_ARCH_INCLUDE_DIR DBUS_LIBRARIES)

endif (DBUS_INCLUDE_DIR AND DBUS_ARCH_INCLUDE_DIR AND DBUS_LIBRARIES)
