#.rst:
# FindLibAACS
# ----------
# Finds the libaacs library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibAACS   - The libaacs library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(PkgConfig QUIET)

  # We only rely on pkgconfig for non windows platforms
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(AACS libaacs QUIET)

    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET AACS_LINK_LIBRARIES 0 AACS_LIBRARY)
  else()
    # todo: for windows use find_package CONFIG call potentially

    find_path(AACS_INCLUDEDIR NAMES libaacs/aacs.h
                              HINTS ${DEPENDS_PATH}/include
                              ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    find_library(AACS_LIBRARY NAMES aacs libaacs
                              HINTS ${DEPENDS_PATH}/lib
                              ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
  endif()

  if(NOT AACS_VERSION AND EXISTS ${AACS_INCLUDEDIR}/libaacs/aacs-version.h)
    file(STRINGS ${AACS_INCLUDEDIR}/libaacs/aacs-version.h _aacs_version_str
         REGEX "#define[ \t]AACS_VERSION_STRING[ \t][\"]?[0-9.]+[\"]?")
    string(REGEX REPLACE "^.*AACS_VERSION_STRING[ \t][\"]?([0-9.]+).*$" "\\1" AACS_VERSION ${_aacs_version_str})
    unset(_aacs_version_str)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibAACS
                                    REQUIRED_VARS AACS_LIBRARY AACS_INCLUDEDIR AACS_VERSION
                                    VERSION_VAR AACS_VERSION)

  if(LIBAACS_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${AACS_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${AACS_INCLUDEDIR}")

  endif()
endif()
