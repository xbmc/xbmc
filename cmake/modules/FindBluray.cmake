#.rst:
# FindBluray
# ----------
# Finds the libbluray library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Bluray   - The libbluray library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libbluray)

  SETUP_FIND_SPECS()

  # Check for existing libcec. If version >= LIBBLURAY-VERSION file version, dont build
  find_package(libbluray ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                         HINTS ${DEPENDS_PATH}/lib/cmake
                         ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # cmake config may not be available (eg Debian libbluray-dev package)
  # fallback to pkgconfig for non windows platforms
  if(NOT libbluray_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})
    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(libbluray libbluray${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  if(TARGET libbluray::libbluray)
    get_target_property(_CEC_CONFIGURATIONS libbluray::libbluray IMPORTED_CONFIGURATIONS)
    foreach(_libbluray_config IN LISTS _CEC_CONFIGURATIONS)
      # Some non standard config (eg None on Debian)
      # Just set to RELEASE var so select_library_configurations can continue to work its magic
      string(TOUPPER ${_libbluray_config} _libbluray_config_UPPER)
      if((NOT ${_libbluray_config_UPPER} STREQUAL "RELEASE") AND
         (NOT ${_libbluray_config_UPPER} STREQUAL "DEBUG"))
        get_target_property(BLURAY_LIBRARY_RELEASE libbluray::libbluray IMPORTED_LOCATION_${_libbluray_config_UPPER})
      else()
        get_target_property(BLURAY_LIBRARY_${_cec_config_UPPER} libbluray::libbluray IMPORTED_LOCATION_${_libbluray_config_UPPER})
      endif()
    endforeach()

    get_target_property(BLURAY_INCLUDE_DIR libbluray::libbluray INTERFACE_INCLUDE_DIRECTORIES)
    set(BLURAY_VERSION ${libbluray_VERSION})
  elseif(TARGET PkgConfig::libbluray)
    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET libbluray_LINK_LIBRARIES 0 BLURAY_LIBRARY_RELEASE)
    get_target_property(BLURAY_INCLUDE_DIR PkgConfig::libbluray INTERFACE_INCLUDE_DIRECTORIES)

    set(BLURAY_VERSION ${libbluray_VERSION})
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(BLURAY)
  unset(BLURAY_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Bluray
                                    REQUIRED_VARS BLURAY_LIBRARY BLURAY_INCLUDE_DIR
                                    VERSION_VAR BLURAY_VERSION)

  if(BLURAY_FOUND)
    if(TARGET libbluray::libbluray)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libbluray::libbluray)
      # We need to append in case the cmake config already has definitions
      set_property(TARGET libbluray::libbluray APPEND PROPERTY
                                                      INTERFACE_COMPILE_DEFINITIONS HAVE_LIBBLURAY)
    elseif(TARGET PkgConfig::libbluray)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::libbluray)
      set_property(TARGET PkgConfig::libbluray APPEND PROPERTY
                                                      INTERFACE_COMPILE_DEFINITIONS HAVE_LIBBLURAY)
    endif()

    # This is incorrectly applied to all platforms. Requires its own handling in the future
    if(NOT CORE_PLATFORM_NAME_LC STREQUAL windowsstore)
      get_property(aliased_target TARGET "${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME}" PROPERTY ALIASED_TARGET)
      set_property(TARGET ${aliased_target} APPEND PROPERTY
                                                   INTERFACE_COMPILE_DEFINITIONS "HAVE_LIBBLURAY_BDJ")
    endif()
  endif()
endif()
