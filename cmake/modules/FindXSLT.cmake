#.rst:
# FindXSLT
# --------
# Finds the XSLT library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::XSLT - The XSLT library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildXSLT)

    find_package(LibXml2 REQUIRED ${SEARCH_QUIET})

    set(XSLT_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    if(WIN32 OR WINDOWS_STORE)
      # xslt only uses debug postfix for windows
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
      set(SHAREDLIB ON)

      set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-win-change_libxml.patch")

      generate_patchcommand("${patches}")

      if(WINDOWS_STORE)
        # Required for UWP
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS /D_CRT_SECURE_NO_WARNINGS)
      endif()

    else()
      set(SHAREDLIB OFF)
    endif()

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=${SHAREDLIB}
                   -DLIBXSLT_WITH_PROGRAMS=OFF
                   -DLIBXSLT_WITH_PYTHON=OFF
                   -DLIBXSLT_WITH_TESTS=OFF
                   ${EXTRA_ARGS})

    BUILD_DEP_TARGET()

    set(XSLT_INCLUDE_DIR ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR})
    set(XSLT_LIBRARY_RELEASE ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE})
    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG)
      set(XSLT_LIBRARY_DEBUG ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG})
    endif()

    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} LibXml2::LibXml2)
  endmacro()

  # If there is a potential this library can be built internally
  # Check its dependencies to allow forcing this lib to be built if one of its
  # dependencies requires being rebuilt
  if(ENABLE_INTERNAL_XSLT)
    # Dependency list of this find module for an INTERNAL build
    set(${CMAKE_FIND_PACKAGE_NAME}_DEPLIST LibXml2)

    check_dependency_build(${CMAKE_FIND_PACKAGE_NAME} "${${CMAKE_FIND_PACKAGE_NAME}_DEPLIST}")
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libxslt)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  # Check for existing libxslt. If version >= LIBXSLT-VERSION file version, dont build
  find_package(libxslt ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                      HINTS ${DEPENDS_PATH}/lib/cmake
                      ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # cmake config may not be available (eg Debian libxslt1-dev package)
  # fallback to pkgconfig for non windows platforms
  if(NOT libxslt_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})
    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
      pkg_check_modules(libxslt libxslt${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  if((libxslt_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_XSLT) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_XSLT) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    # Build lib
    buildXSLT()
  else()
    if(TARGET LibXslt::LibXslt)
      get_target_property(_XSLT_CONFIGURATIONS LibXslt::LibXslt IMPORTED_CONFIGURATIONS)
      foreach(_xslt_config IN LISTS _XSLT_CONFIGURATIONS)
        # Some non standard config (eg None on Debian)
        # Just set to RELEASE var so select_library_configurations can continue to work its magic
        string(TOUPPER ${_xslt_config} _xslt_config_UPPER)
        if((NOT ${_xslt_config_UPPER} STREQUAL "RELEASE") AND
           (NOT ${_xslt_config_UPPER} STREQUAL "DEBUG"))
          get_target_property(XSLT_LIBRARY_RELEASE LibXslt::LibXslt IMPORTED_LOCATION_${_xslt_config_UPPER})
        else()
          get_target_property(XSLT_LIBRARY_${_xslt_config_UPPER} LibXslt::LibXslt IMPORTED_LOCATION_${_xslt_config_UPPER})
        endif()
      endforeach()

      get_target_property(XSLT_INCLUDE_DIR LibXslt::LibXslt INTERFACE_INCLUDE_DIRECTORIES)
      set(XSLT_VERSION ${libxslt_VERSION})
    elseif(TARGET PkgConfig::libxslt)
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET libxslt_LINK_LIBRARIES 0 XSLT_LIBRARY_RELEASE)

      get_target_property(XSLT_INCLUDE_DIR PkgConfig::libxslt INTERFACE_INCLUDE_DIRECTORIES)
      set(XSLT_VERSION ${libxslt_VERSION})
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(XSLT)
  unset(XSLT_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(XSLT
                                    REQUIRED_VARS XSLT_LIBRARY XSLT_INCLUDE_DIR
                                    VERSION_VAR XSLT_VERSION)

  if(XSLT_FOUND)
    if(TARGET LibXslt::LibXslt AND NOT TARGET xslt)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS LibXslt::LibXslt)
      # We need to append in case the cmake config already has definitions
      set_property(TARGET LibXslt::LibXslt APPEND PROPERTY
                                                  INTERFACE_COMPILE_DEFINITIONS HAVE_LIBXSLT)
    elseif(TARGET PkgConfig::libxslt AND NOT TARGET xslt)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::libxslt)
      set_property(TARGET PkgConfig::libxslt APPEND PROPERTY
                                                    INTERFACE_COMPILE_DEFINITIONS HAVE_LIBXSLT)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${XSLT_INCLUDE_DIR}"
                                                                       INTERFACE_COMPILE_DEFINITIONS HAVE_LIBXSLT)

      if(XSLT_LIBRARY_RELEASE)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                                         IMPORTED_LOCATION_RELEASE "${XSLT_LIBRARY_RELEASE}")
      endif()
      if(XSLT_LIBRARY_DEBUG)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_LOCATION_DEBUG "${XSLT_LIBRARY_DEBUG}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              IMPORTED_CONFIGURATIONS DEBUG)
      endif()

      target_link_libraries(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE LibXml2::LibXml2)
    endif()

    if(TARGET libxslt)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} libxslt)
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET libxslt)
        buildXSLT()
        set_target_properties(libxslt PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends libxslt)
    endif()
  else()
    if(XSLT_FIND_REQUIRED)
      message(FATAL_ERROR "XSLT library was not found. You may want to try -DENABLE_INTERNAL_XSLT=ON")
    endif()
  endif()
endif()
