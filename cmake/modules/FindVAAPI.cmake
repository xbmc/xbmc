#.rst:
# FindVAAPI
# ---------
# Finds the VAAPI library
#
# This will define the following variables::
#
# VAAPI_FOUND - system has VAAPI
# VAAPI_INCLUDE_DIRS - the VAAPI include directory
# VAAPI_LIBRARIES - the VAAPI libraries
# VAAPI_DEFINITIONS - the VAAPI definitions

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_VAAPI libva libva-drm libva-wayland libva-x11 QUIET)
endif()

set(REQUIRED_VARS "VAAPI_libva_LIBRARY" "VAAPI_libva-drm_LIBRARY" "VAAPI_libva_INCLUDE_DIR" "VAAPI_libva-drm_INCLUDE_DIR")

find_path(VAAPI_libva_INCLUDE_DIR va/va.h
                            HINTS ${PC_VAAPI_libva_INCLUDEDIR})
find_library(VAAPI_libva_LIBRARY NAMES va
                                 HINTS ${PC_VAAPI_libva_LIBDIR})
find_path(VAAPI_libva-drm_INCLUDE_DIR va/va_drm.h
                                HINTS ${PC_VAAPI_libva-drm_INCLUDEDIR})
find_library(VAAPI_libva-drm_LIBRARY NAMES va-drm
                                     HINTS ${PC_VAAPI_libva-drm_LIBDIR})
if("wayland" IN_LIST CORE_PLATFORM_NAME_LC)
  find_path(VAAPI_libva-wayland_INCLUDE_DIR va/va_wayland.h
                                      HINTS ${PC_VAAPI_libva-wayland_INCLUDEDIR})
  find_library(VAAPI_libva-wayland_LIBRARY NAMES va-wayland
                                           HINTS ${PC_VAAPI_libva-wayland_LIBDIR})
  list(APPEND REQUIRED_VARS "VAAPI_libva-wayland_INCLUDE_DIR" "VAAPI_libva-wayland_LIBRARY")
endif()
if("x11" IN_LIST CORE_PLATFORM_NAME_LC)
  find_path(VAAPI_libva-x11_INCLUDE_DIR va/va_x11.h
                                  HINTS ${PC_VAAPI_libva-x11_INCLUDEDIR})
  find_library(VAAPI_libva-x11_LIBRARY NAMES va-x11
                                       HINTS ${PC_VAAPI_libva-x11_LIBDIR})
  list(APPEND REQUIRED_VARS "VAAPI_libva-x11_INCLUDE_DIR" "VAAPI_libva-x11_LIBRARY")
endif()

if(PC_VAAPI_libva_VERSION)
  set(VAAPI_VERSION_STRING ${PC_VAAPI_libva_VERSION})
elseif(VAAPI_INCLUDE_DIR AND EXISTS "${VAAPI_INCLUDE_DIR}/va/va_version.h")
  file(STRINGS "${VAAPI_INCLUDE_DIR}/va/va_version.h" vaapi_version_str REGEX "^#define[\t ]+VA_VERSION_S[\t ]+\".*\".*")
  string(REGEX REPLACE "^#define[\t ]+VA_VERSION_S[\t ]+\"([^\"]+)\".*" "\\1" VAAPI_VERSION_STRING "${vaapi_version_str}")
  unset(vaapi_version_str)
endif()

if(NOT VAAPI_FIND_VERSION)
  set(VAAPI_FIND_VERSION 0.39.0)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VAAPI
                                  REQUIRED_VARS ${REQUIRED_VARS}
                                  VERSION_VAR VAAPI_VERSION_STRING)

if(VAAPI_FOUND)
  set(VAAPI_INCLUDE_DIRS ${VAAPI_INCLUDE_DIR} ${VAAPI_DRM_INCLUDE_DIR} ${VAAPI_WAYLAND_INCLUDE_DIR} ${VAAPI_X11_INCLUDE_DIR})
  set(VAAPI_LIBRARIES ${VAAPI_libva_LIBRARY} ${VAAPI_libva-drm_LIBRARY} ${VAAPI_libva-wayland_LIBRARY} ${VAAPI_libva-x11_LIBRARY})
  set(VAAPI_DEFINITIONS -DHAVE_LIBVA=1)
endif()

mark_as_advanced(VAAPI_libva_INCLUDE_DIR VAAPI_libva-drm_INCLUDE_DIR VAAPI_libva-wayland_INCLUDE_DIR VAAPI_libva-x11_INCLUDE_DIR
                 VAAPI_libva_LIBRARY VAAPI_libva-drm_LIBRARY VAAPI_libva-wayland_LIBRARY VAAPI_libva-x11_LIBRARY)
