#.rst:
# FindVAAPI
# ---------
# Finds the VAAPI library
#
# This will will define the following variables::
#
# VAAPI_FOUND - system has VAAPI
# VAAPI_INCLUDE_DIRS - the VAAPI include directory
# VAAPI_LIBRARIES - the VAAPI libraries
# VAAPI_DEFINITIONS - the VAAPI definitions
#
# and the following imported targets::
#
#   VAAPI::VAAPI   - The VAAPI library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_VAAPI libva libva-x11 QUIET)
endif()

find_path(VAAPI_INCLUDE_DIR va/va.h
                            PATHS ${PC_VAAPI_libva_INCLUDEDIR})
find_library(VAAPI_libva_LIBRARY NAMES va
                                 PATHS ${PC_VAAPI_libva_LIBDIR})
find_library(VAAPI_libva-x11_LIBRARY NAMES va-x11
                                     PATHS ${PC_VAAPI_libva_LIBDIR})

if(PC_VAAPI_libva_VERSION)
  set(VAAPI_VERSION_STRING ${PC_VAAPI_libva_VERSION})
elseif(VAAPI_INCLUDE_DIR AND EXISTS "${VAAPI_INCLUDE_DIR}/va/va_version.h")
  file(STRINGS "${VAAPI_INCLUDE_DIR}/va/va_version.h" vaapi_version_str REGEX "^#define[\t ]+VA_VERSION_S[\t ]+\".*\".*")
  string(REGEX REPLACE "^#define[\t ]+VA_VERSION_S[\t ]+\"([^\"]+)\".*" "\\1" VAAPI_VERSION_STRING "${vaapi_version_str}")
  unset(vaapi_version_str)
endif()

if(NOT VAAPI_FIND_VERSION)
  set(VAAPI_FIND_VERSION 0.38.0)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VAAPI
                                  REQUIRED_VARS VAAPI_libva_LIBRARY VAAPI_libva-x11_LIBRARY VAAPI_INCLUDE_DIR
                                  VERSION_VAR VAAPI_VERSION_STRING)

if(VAAPI_FOUND)
  set(VAAPI_INCLUDE_DIRS ${VAAPI_INCLUDE_DIR})
  set(VAAPI_LIBRARIES ${VAAPI_libva_LIBRARY} ${VAAPI_libva-x11_LIBRARY})
  set(VAAPI_DEFINITIONS -DHAVE_LIBVA=1)

  if(NOT TARGET VAAPI::VAAPI_X11)
    add_library(VAAPI::VAAPI_X11 UNKNOWN IMPORTED)
    set_target_properties(VAAPI::VAAPI_X11 PROPERTIES
                                           IMPORTED_LOCATION "${VAAPI_libva-x11_LIBRARY}")
  endif()
  if(NOT TARGET VAAPI::VAAPI)
    add_library(VAAPI::VAAPI UNKNOWN IMPORTED)
    set_target_properties(VAAPI::VAAPI PROPERTIES
                                       IMPORTED_LOCATION "${VAAPI_libva_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${VAAPI_INCLUDE_DIR}"
                                       INTERFACE_COMPILE_DEFINITIONS HAVE_LIBVA=1
                                       INTERFACE_LINK_LIBRARIES VAAPI::VAAPI_X11)
  endif()
endif()

mark_as_advanced(VAAPI_INCLUDE_DIR VAAPI_libva_LIBRARY VAAPI_libva-x11_LIBRARY)
