#.rst:
# FindNGHttp2
# ----------
# Finds the NGHttp2 library
#
# This will define the following target:
#
#   LIBRARY::NGHttp2   - The App specific library dependency target
#

if(NOT TARGET LIBRARY::NGHttp2)

  macro(buildlibnghttp2)

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-all-cmake-version.patch")
    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DENABLE_DEBUG=OFF
                   -DENABLE_FAILMALLOC=OFF
                   -DENABLE_LIB_ONLY=ON
                   -DENABLE_DOC=OFF
                   -DBUILD_STATIC_LIBS=ON
                   -DBUILD_SHARED_LIBS=OFF
                   -DWITH_LIBXML2=OFF)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC nghttp2)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  # Search for cmake config. Suitable for all platforms including windows
  # nghttp uses a non standard config name, so we have to supply CONFIGS
  find_package(nghttp2 ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                       CONFIGS nghttp2-targets.cmake
                       HINTS ${DEPENDS_PATH}/lib/cmake
                       ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # cmake config may not be available (eg Debian libnghttp2-dev package)
  # fallback to pkgconfig for non windows platforms
  if(NOT nghttp2_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(nghttp2 libnghttp2${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  # Check for existing Nghttp2. If version >= NGHTTP2-VERSION file version, dont build
  if(nghttp2_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND NGHttp2_FIND_REQUIRED)
    # Build lib
    buildlibnghttp2()
  else()
    if(TARGET nghttp2::nghttp2 OR TARGET nghttp2::nghttp2_static)

      # We have a preference for the static lib when needed, however provide support for the shared
      # lib as well
      if(TARGET nghttp2::nghttp2_static)
        set(_target nghttp2::nghttp2_static)
      else()
        set(_target nghttp2::nghttp2)
      endif()

      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_NGHTTP2_CONFIGURATIONS ${_target} IMPORTED_CONFIGURATIONS)
      foreach(_nghttp2_config IN LISTS _NGHTTP2_CONFIGURATIONS)
        # Some non standard config (eg None on Debian)
        # Just set to RELEASE var so select_library_configurations can continue to work its magic
        string(TOUPPER ${_nghttp2_config} _nghttp2_config_UPPER)
        if((NOT ${_nghttp2_config_UPPER} STREQUAL "RELEASE") AND
           (NOT ${_nghttp2_config_UPPER} STREQUAL "DEBUG"))
          get_target_property(NGHTTP2_LIBRARY_RELEASE ${_target} IMPORTED_LOCATION_${_nghttp2_config_UPPER})
        else()
          get_target_property(NGHTTP2_LIBRARY_${_nghttp2_config_UPPER} ${_target} IMPORTED_LOCATION_${_nghttp2_config_UPPER})
        endif()
      endforeach()

      get_target_property(NGHTTP2_INCLUDE_DIR ${_target} INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::nghttp2)
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET nghttp2_LINK_LIBRARIES 0 NGHTTP2_LIBRARY_RELEASE)

      get_target_property(NGHTTP2_INCLUDE_DIR PkgConfig::nghttp2 INTERFACE_INCLUDE_DIRECTORIES)

      set(NGHTTP2_VERSION ${nghttp2_VERSION})
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(NGHTTP2)
  unset(NGHTTP2_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(NGHttp2
                                    REQUIRED_VARS NGHTTP2_LIBRARY NGHTTP2_INCLUDE_DIR
                                    VERSION_VAR NGHTTP2_VERSION)

  if(NGHTTP2_FOUND)

    if((TARGET nghttp2::nghttp2 OR TARGET nghttp2::nghttp2_static) AND NOT TARGET nghttp2)
      # We have a preference for the static lib when needed, however provide support 
      # for the shared lib as well
      if(TARGET nghttp2::nghttp2_static)
        set(_target nghttp2::nghttp2_static)
      else()
        set(_target nghttp2::nghttp2)
      endif()

      # This is a kodi target name
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_target})
    elseif(TARGET PkgConfig::nghttp2 AND NOT TARGET nghttp2)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::nghttp2)
    else()

      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
  
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                INTERFACE_COMPILE_DEFINITIONS "NGHTTP2_STATICLIB"
                                                                INTERFACE_INCLUDE_DIRECTORIES "${NGHTTP2_INCLUDE_DIR}")

      if(NGHTTP2_LIBRARY_RELEASE)
        set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                  IMPORTED_CONFIGURATIONS RELEASE
                                                                  IMPORTED_LOCATION_RELEASE "${NGHTTP2_LIBRARY_RELEASE}")
      endif()
      if(NGHTTP2_LIBRARY_DEBUG)
        set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                  IMPORTED_LOCATION_DEBUG "${NGHTTP2_LIBRARY_DEBUG}")
        set_property(TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                       IMPORTED_CONFIGURATIONS DEBUG)
      endif()
    endif()

    if(TARGET nghttp2)
      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} nghttp2)

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
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
      if(NOT TARGET nghttp2)
        buildlibnghttp2()
        set_target_properties(nghttp2 PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends nghttp2)
    endif()
  else()
    if(NGHttp2_FIND_REQUIRED)
      message(FATAL_ERROR "NGHttp2 libraries were not found.")
    endif()
  endif()
endif()
