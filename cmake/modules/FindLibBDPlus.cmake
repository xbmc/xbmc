#.rst:
# FindLibBDPlus
# ----------
# Finds the libbdplus library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibBDPlus   - The libbdplus library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(PkgConfig ${SEARCH_QUIET})

  # We only rely on pkgconfig for non windows platforms
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(BDPLUS libbdplus ${SEARCH_QUIET})

    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET BDPLUS_LINK_LIBRARIES 0 BDPLUS_LIBRARY)
  else()
    # todo: for windows use find_package CONFIG call potentially

    find_path(BDPLUS_INCLUDEDIR NAMES libbdplus/bdplus.h
                                HINTS ${DEPENDS_PATH}/include
                                ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    find_library(BDPLUS_LIBRARY NAMES bdplus libbdplus
                              HINTS ${DEPENDS_PATH}/lib
                              ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
  endif()

  if(NOT BDPLUS_VERSION AND EXISTS ${BDPLUS_INCLUDEDIR}/libbdplus/bdplus-version.h)
    file(STRINGS ${BDPLUS_INCLUDEDIR}/libbdplus/bdplus-version.h _bdplus_version_str
         REGEX "#define[ \t]BDPLUS_VERSION_STRING[ \t][\"]?[0-9.]+[\"]?")
    string(REGEX REPLACE "^.*BDPLUS_VERSION_STRING[ \t][\"]?([0-9.]+).*$" "\\1" BDPLUS_VERSION ${_bdplus_version_str})
    unset(_bdplus_version_str)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibBDPlus
                                    REQUIRED_VARS BDPLUS_LIBRARY BDPLUS_INCLUDEDIR BDPLUS_VERSION
                                    VERSION_VAR BDPLUS_VERSION)

  if(LIBBDPLUS_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${BDPLUS_INCLUDEDIR}"
                                                                     INTERFACE_LINK_LIBRARIES "${BDPLUS_LIBRARY}")
  endif()
endif()
