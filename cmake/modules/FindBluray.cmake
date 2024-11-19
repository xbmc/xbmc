#.rst:
# FindBluray
# ----------
# Finds the libbluray library
#
# This will define the following variables::
#
# BLURAY_FOUND - system has libbluray
# BLURAY_INCLUDE_DIRS - the libbluray include directory
# BLURAY_LIBRARIES - the libbluray libraries
# BLURAY_DEFINITIONS - the libbluray compile definitions
#
# and the following imported targets::
#
#   Bluray::Bluray   - The libbluray library

if(Bluray_FIND_VERSION)
  if(Bluray_FIND_VERSION_EXACT)
    set(Bluray_FIND_SPEC "=${Bluray_FIND_VERSION_COMPLETE}")
  else()
    set(Bluray_FIND_SPEC ">=${Bluray_FIND_VERSION_COMPLETE}")
  endif()
endif()

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_BLURAY libbluray${Bluray_FIND_SPEC} QUIET)
  set(BLURAY_VERSION ${PC_BLURAY_VERSION})
endif()

find_path(BLURAY_INCLUDE_DIR libbluray/bluray.h
                             HINTS ${PC_BLURAY_INCLUDEDIR})

if(NOT BLURAY_VERSION AND EXISTS ${BLURAY_INCLUDE_DIR}/libbluray/bluray-version.h)
  file(STRINGS ${BLURAY_INCLUDE_DIR}/libbluray/bluray-version.h _bluray_version_str
       REGEX "#define[ \t]BLURAY_VERSION_STRING[ \t][\"]?[0-9.]+[\"]?")
  string(REGEX REPLACE "^.*BLURAY_VERSION_STRING[ \t][\"]?([0-9.]+).*$" "\\1" BLURAY_VERSION ${_bluray_version_str})
  unset(_bluray_version_str)
endif()

find_library(BLURAY_LIBRARY NAMES bluray libbluray
                            HINTS ${PC_BLURAY_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bluray
                                  REQUIRED_VARS BLURAY_LIBRARY BLURAY_INCLUDE_DIR BLURAY_VERSION
                                  VERSION_VAR BLURAY_VERSION)

if(BLURAY_FOUND)
  set(BLURAY_LIBRARIES ${BLURAY_LIBRARY})
  set(BLURAY_INCLUDE_DIRS ${BLURAY_INCLUDE_DIR})
  set(BLURAY_DEFINITIONS -DHAVE_LIBBLURAY=1)

  # todo: improve syntax
  if (NOT CORE_PLATFORM_NAME_LC STREQUAL windowsstore)
    list(APPEND BLURAY_DEFINITIONS -DHAVE_LIBBLURAY_BDJ=1)
  endif()

  if(${BLURAY_LIBRARY} MATCHES ".+\.a$" AND PC_BLURAY_STATIC_LIBRARIES)
    list(APPEND BLURAY_LIBRARIES ${PC_BLURAY_STATIC_LIBRARIES})
  endif()

  if(NOT TARGET Bluray::Bluray)
    add_library(Bluray::Bluray UNKNOWN IMPORTED)
    if(BLURAY_LIBRARY)
      set_target_properties(Bluray::Bluray PROPERTIES
                                           IMPORTED_LOCATION "${BLURAY_LIBRARY}")
    endif()
    set_target_properties(Bluray::Bluray PROPERTIES
                                         INTERFACE_INCLUDE_DIRECTORIES "${BLURAY_INCLUDE_DIR}"
                                         INTERFACE_COMPILE_DEFINITIONS HAVE_LIBBLURAY=1)
  endif()
endif()

mark_as_advanced(BLURAY_INCLUDE_DIR BLURAY_LIBRARY)
