#.rst:
# FindUdfread
# --------
# Finds the udfread library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Udfread - The libudfread library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildudfread)
    set(UDFREAD_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    set(BUILD_NAME udfread_build)

    set(CONFIGURE_COMMAND autoreconf -vif &&
                          ./configure
                          --enable-static
                          --disable-shared
                          --prefix=${DEPENDS_PATH})
    set(BUILD_IN_SOURCE 1)

    BUILD_DEP_TARGET()
  endmacro()

  if(WIN32 OR WINDOWS_STORE)
    include(FindPackageMessage)

    find_package(libudfread CONFIG QUIET
                            HINTS ${DEPENDS_PATH}/lib/cmake
                            ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

    if(libudfread_FOUND)
      # Specifically tailored to kodi windows cmake config - Debug and RelWithDebInfo available
      get_target_property(UDFREAD_LIBRARY_RELEASE libudfread::libudfread IMPORTED_LOCATION_RELWITHDEBINFO)
      get_target_property(UDFREAD_LIBRARY_DEBUG libudfread::libudfread IMPORTED_LOCATION_DEBUG)
      get_target_property(UDFREAD_INCLUDE_DIR libudfread::libudfread INTERFACE_INCLUDE_DIRECTORIES)

      include(SelectLibraryConfigurations)
      select_library_configurations(UDFREAD)
      unset(UDFREAD_LIBRARIES)
    endif()
  else()
    find_package(PkgConfig QUIET)
    pkg_check_modules(libudfread libudfread IMPORTED_TARGET GLOBAL QUIET)

    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC udfread)
    SETUP_BUILD_VARS()

    # Check for existing UDFREAD. If version >= UDFREAD-VERSION file version, dont build
    # A corner case, but if a linux/freebsd user WANTS to build internal udfread, build anyway
    if((libudfread_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_UDFREAD) OR
       ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_UDFREAD))
      buildudfread()
  
    else()
      if(TARGET PkgConfig::libudfread)
        get_target_property(UDFREAD_LIBRARY PkgConfig::libudfread INTERFACE_LINK_LIBRARIES)
        get_target_property(UDFREAD_INCLUDE_DIR PkgConfig::libudfread INTERFACE_INCLUDE_DIRECTORIES)
        set(UDFREAD_VERSION ${libudfread_VERSION})
      endif()
    endif()
  endif()

  find_package_handle_standard_args(Udfread
                                    REQUIRED_VARS UDFREAD_LIBRARY UDFREAD_INCLUDE_DIR
                                    VERSION_VAR UDFREAD_VERSION)

  if(UDFREAD_FOUND)
    # pkgconfig populate target that is sufficient version
    if(TARGET PkgConfig::libudfread AND NOT TARGET udfread_build)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::libudfread)
      set_target_properties(PkgConfig::libudfread PROPERTIES
                                                  INTERFACE_COMPILE_DEFINITIONS HAS_UDFREAD)
    # windows cmake config populated target
    elseif(TARGET libudfread::libudfread)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libudfread::libudfread)
      set_target_properties(libudfread::libudfread PROPERTIES
                                                   INTERFACE_COMPILE_DEFINITIONS HAS_UDFREAD)
    # otherwise we are building
    elseif(TARGET udfread_build)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION "${UDFREAD_LIBRARY}"
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${UDFREAD_INCLUDE_DIR}"
                                                                       INTERFACE_COMPILE_DEFINITIONS HAS_UDFREAD)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} udfread_build)
    endif()
  endif()
endif()
