#.rst:
# FindUdfread
# --------
# Finds the udfread library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Udfread - The libudfread library
#   LIBRARY::Udfread - ALIAS target for the libudfread library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC udfread)

  SETUP_FIND_SPECS()

  # Windows only provides a cmake config. This is custom to us
  find_package(libudfread ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                          HINTS ${DEPENDS_PATH}/lib/cmake
                          ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # fallback to pkgconfig for non windows platforms
  if(NOT libudfread_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(libudfread libudfread${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  if(TARGET libudfread::libudfread)
    get_target_property(UDFREAD_LIBRARY libudfread::libudfread IMPORTED_LOCATION_RELWITHDEBINFO)
    get_target_property(UDFREAD_INCLUDE_DIR libudfread::libudfread INTERFACE_INCLUDE_DIRECTORIES)
  elseif(TARGET PkgConfig::libudfread)
    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET libudfread_LINK_LIBRARIES 0 UDFREAD_LIBRARY)

    get_target_property(UDFREAD_INCLUDE_DIR PkgConfig::libudfread INTERFACE_INCLUDE_DIRECTORIES)
    set(UDFREAD_VERSION ${libudfread_VERSION})
  endif()

  find_package_handle_standard_args(Udfread
                                    REQUIRED_VARS UDFREAD_LIBRARY UDFREAD_INCLUDE_DIR
                                    VERSION_VAR UDFREAD_VERSION)

  if(UDFREAD_FOUND)
    # windows cmake config populated target
    if(TARGET libudfread::libudfread)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libudfread::libudfread)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS libudfread::libudfread)
      set_target_properties(libudfread::libudfread PROPERTIES
                                                   INTERFACE_COMPILE_DEFINITIONS HAS_UDFREAD)
    # pkgconfig populated target that is sufficient version
    elseif(TARGET PkgConfig::libudfread)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::libudfread)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::libudfread)
      set_target_properties(PkgConfig::libudfread PROPERTIES
                                                  INTERFACE_COMPILE_DEFINITIONS HAS_UDFREAD)
    endif()
  else()
    if(Udfread_FIND_REQUIRED)
      message(FATAL_ERROR "Udfread libraries were not found.")
    endif()
  endif()
endif()
