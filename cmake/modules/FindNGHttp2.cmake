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

  macro(buildmacroNGHttp2)

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
  find_package(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                                                         CONFIGS nghttp2-targets.cmake
                                                         HINTS ${DEPENDS_PATH}/lib/cmake
                                                         ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

  # cmake config may not be available (eg Debian libnghttp2-dev package)
  # fallback to pkgconfig for non windows platforms
  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} libnghttp2${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  # Check for existing Nghttp2. If version >= NGHTTP2-VERSION file version, dont build
  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND NGHttp2_FIND_REQUIRED)
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
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
      if(_NGHTTP2_CONFIGURATIONS)
        foreach(_nghttp2_config IN LISTS _NGHTTP2_CONFIGURATIONS)
          # Some non standard config (eg None on Debian)
          # Just set to RELEASE var so select_library_configurations can continue to work its magic
          string(TOUPPER ${_nghttp2_config} _nghttp2_config_UPPER)
          if((NOT ${_nghttp2_config_UPPER} STREQUAL "RELEASE") AND
             (NOT ${_nghttp2_config_UPPER} STREQUAL "DEBUG"))
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE ${_target} IMPORTED_LOCATION_${_nghttp2_config_UPPER})
          else()
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_${_nghttp2_config_UPPER} ${_target} IMPORTED_LOCATION_${_nghttp2_config_UPPER})
          endif()
        endforeach()
      else()
        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE ${_target} IMPORTED_LOCATION)
      endif()

      get_target_property(NGHTTP2_INCLUDE_DIR ${_target} INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION})
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(NGHttp2
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(NGHttp2_FOUND)

    if((TARGET nghttp2::nghttp2 OR TARGET nghttp2::nghttp2_static) AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # We have a preference for the static lib when needed, however provide support 
      # for the shared lib as well
      if(TARGET nghttp2::nghttp2_static)
        set(_target nghttp2::nghttp2_static)
      else()
        set(_target nghttp2::nghttp2)
      endif()

      # This is a kodi target name
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_target})
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE LIBRARY)
      SETUP_BUILD_TARGET()

      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(NGHttp2_FIND_REQUIRED)
      message(FATAL_ERROR "NGHttp2 libraries were not found.")
    endif()
  endif()
endif()
