#.rst:
# FindVAAPI
# ---------
# Finds the VAAPI library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::VAAPI   - The VAAPI library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
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
    if(VAAPI_libva-x11_LIBRARY)
      add_library(${APP_NAME_LC}::va-x11 UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::va-x11 PROPERTIES
                                                   IMPORTED_LOCATION "${VAAPI_libva-x11_LIBRARY}"
                                                   INTERFACE_INCLUDE_DIRECTORIES "${VAAPI_libva-x11_INCLUDE_DIR}")
    endif()

    if(VAAPI_libva-wayland_LIBRARY)
      add_library(${APP_NAME_LC}::va-wayland UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::va-wayland PROPERTIES
                                                       IMPORTED_LOCATION "${VAAPI_libva-wayland_LIBRARY}"
                                                       INTERFACE_INCLUDE_DIRECTORIES "${VAAPI_libva-wayland_INCLUDE_DIR}")
    endif()

    add_library(${APP_NAME_LC}::va-drm UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::va-drm PROPERTIES
                                                     IMPORTED_LOCATION "${VAAPI_libva-drm_LIBRARY}"
                                                     INTERFACE_INCLUDE_DIRECTORIES "${VAAPI_libva-drm_INCLUDE_DIR}")

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${VAAPI_libva_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${VAAPI_libva_INCLUDE_DIR}"
                                                                     INTERFACE_LINK_LIBRARIES "${APP_NAME_LC}::va-drm;$<TARGET_NAME_IF_EXISTS:${APP_NAME_LC}::va-wayland>;$<TARGET_NAME_IF_EXISTS:${APP_NAME_LC}::va-x11>"
                                                                     INTERFACE_COMPILE_DEFINITIONS "HAVE_LIBVA")
  endif()
endif()
