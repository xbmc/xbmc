#.rst:
# FindLibAACS
# ----------
# Finds the libaacs library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibAACS   - The libaacs library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(PkgConfig ${SEARCH_QUIET})

  # We only rely on pkgconfig for non windows platforms
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(LIBAACS libaacs IMPORTED_TARGET ${SEARCH_QUIET})

    get_target_property(LIBAACS_LIBRARY PkgConfig::LIBAACS INTERFACE_LINK_LIBRARIES)
    get_target_property(LIBAACS_INCLUDE_DIR PkgConfig::LIBAACS INTERFACE_INCLUDE_DIRECTORIES)
  else()
    # Current packaged windows libaacs cmake config has a fault, so we cant use find_package
    find_path(LIBAACS_INCLUDE_DIR NAMES libaacs/aacs.h
                                  HINTS ${DEPENDS_PATH}/include
                                  ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    find_library(LIBAACS_LIBRARY NAMES aacs libaacs
                                 HINTS ${DEPENDS_PATH}/lib
                                 ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    if(EXISTS ${LIBAACS_INCLUDEDIR}/libaacs/aacs-version.h)
      file(STRINGS ${LIBAACS_INCLUDEDIR}/libaacs/aacs-version.h _aacs_version_str
           REGEX "#define[ \t]AACS_VERSION_STRING[ \t][\"]?[0-9.]+[\"]?")
      string(REGEX REPLACE "^.*AACS_VERSION_STRING[ \t][\"]?([0-9.]+).*$" "\\1" LIBAACS_VERSION ${_aacs_version_str})
      unset(_aacs_version_str)
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibAACS
                                    REQUIRED_VARS LIBAACS_LIBRARY LIBAACS_INCLUDE_DIR
                                    VERSION_VAR LIBAACS_VERSION)

  if(LIBAACS_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBAACS_INCLUDE_DIR}"
                                                                     INTERFACE_LINK_LIBRARIES "${LIBAACS_LIBRARY}")
  endif()
endif()
