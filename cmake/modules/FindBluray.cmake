#.rst:
# FindBluray
# ----------
# Finds the libbluray library
#
# This will will define the following variables::
#
# BLURAY_FOUND - system has libbluray
# BLURAY_INCLUDE_DIRS - the libbluray include directory
# BLURAY_LIBRARIES - the libbluray libraries
# BLURAY_DEFINITIONS - the libbluray compile definitions
#
# and the following imported targets::
#
#   Bluray::Bluray   - The libblueray library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_BLURAY libbluray>=0.7.0 QUIET)
endif()

find_path(BLURAY_INCLUDE_DIR libbluray/bluray.h
                             PATHS ${PC_BLURAY_INCLUDEDIR})

set(BLURAY_VERSION ${PC_BLURAY_VERSION})

include(FindPackageHandleStandardArgs)
if(NOT WIN32)
  find_library(BLURAY_LIBRARY NAMES bluray
                              PATHS ${PC_BLURAY_LIBDIR})

  find_package_handle_standard_args(Bluray
                                    REQUIRED_VARS BLURAY_LIBRARY BLURAY_INCLUDE_DIR
                                    VERSION_VAR BLURAY_VERSION)
else()
  # Dynamically loaded DLL
  find_package_handle_standard_args(Bluray
                                    REQUIRED_VARS BLURAY_INCLUDE_DIR
                                    VERSION_VAR BLURAY_VERSION)
endif()

if(BLURAY_FOUND)
  set(BLURAY_LIBRARIES ${BLURAY_LIBRARY})
  set(BLURAY_INCLUDE_DIRS ${BLURAY_INCLUDE_DIR})
  set(BLURAY_DEFINITIONS -DHAVE_LIBBLURAY=1)

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
