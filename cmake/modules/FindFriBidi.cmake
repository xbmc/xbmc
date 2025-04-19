#.rst:
# FindFriBidi
# -----------
# Finds the GNU FriBidi library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::FriBidi   - The FriBidi library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})

  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(FRIBIDI fribidi IMPORTED_TARGET GLOBAL ${SEARCH_QUIET})

    get_target_property(FRIBIDI_LIBRARY PkgConfig::FRIBIDI INTERFACE_LINK_LIBRARIES)
    get_target_property(FRIBIDI_INCLUDE_DIR PkgConfig::FRIBIDI INTERFACE_INCLUDE_DIRECTORIES)

  else()
    find_path(FRIBIDI_INCLUDE_DIR NAMES fribidi.h
                                  PATH_SUFFIXES fribidi
                                  HINTS ${DEPENDS_PATH}/include)
    find_library(FRIBIDI_LIBRARY NAMES fribidi libfribidi
                                 HINTS ${DEPENDS_PATH}/lib)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(FriBidi
                                    REQUIRED_VARS FRIBIDI_LIBRARY FRIBIDI_INCLUDE_DIR
                                    VERSION_VAR FRIBIDI_VERSION)

  if(FRIBIDI_FOUND)
    if(TARGET PkgConfig::FRIBIDI)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::FRIBIDI)
      if(NOT TARGET FriBidi::FriBidi)
        add_library(FriBidi::FriBidi ALIAS PkgConfig::FRIBIDI)
      endif()
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION "${FRIBIDI_LIBRARY}"
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${FRIBIDI_INCLUDE_DIR}")
      if(NOT TARGET FriBidi::FriBidi)
        add_library(FriBidi::FriBidi ALIAS ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
      endif()
    endif()
  else()
    if(FriBidi_FIND_REQUIRED)
      message(FATAL_ERROR "FriBidi library was not found.")
    endif()
  endif()
endif()
