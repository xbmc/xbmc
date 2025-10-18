#.rst:
# FindCurl
# --------
# Finds the Curl library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Curl   - The Curl library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroCurl)
    find_package(Brotli REQUIRED ${SEARCH_QUIET})
    find_package(NGHttp2 REQUIRED ${SEARCH_QUIET})
    find_package(OpenSSL REQUIRED ${SEARCH_QUIET})

    # Darwin platforms link against toolchain provided zlib regardless
    # They will fail when searching for static. All other platforms, prefer static
    # if possible (requires cmake 3.24+ otherwise variable is a no-op)
    # Windows still uses dynamic lib for zlib for other purposes, dont mix
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" AND NOT (WIN32 OR WINDOWS_STORE))
      set(ZLIB_USE_STATIC_LIBS ON)
    endif()
    find_package(ZLIB REQUIRED ${SEARCH_QUIET})
    unset(ZLIB_USE_STATIC_LIBS)

    set(CURL_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    # Curl debug uses postfix -d for all platforms
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX -d)

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS -DNGHTTP2_STATICLIB)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES crypt32.lib)
    endif()

    set(CMAKE_ARGS -DBUILD_CURL_EXE=OFF
                   -DBUILD_SHARED_LIBS=OFF
                   -DBUILD_STATIC_LIBS=ON
                   -DBUILD_LIBCURL_DOCS=OFF
                   -DENABLE_CURL_MANUAL=OFF
                   -DCURL_DISABLE_TESTS=OFF
                   -DCURL_DISABLE_LDAP=ON
                   -DCURL_DISABLE_LDAPS=ON
                   -DCURL_DISABLE_SMB=OFF
                   -DCURL_USE_OPENSSL=ON
                   -DOPENSSL_ROOT_DIR=${DEPENDS_PATH}
                   -DCURL_BROTLI=ON
                   -DUSE_NGHTTP2=ON
                   -DUSE_LIBIDN2=OFF
                   -DCURL_USE_LIBSSH2=OFF
                   -DCURL_USE_GSSAPI=OFF
                   -DCURL_CA_FALLBACK=ON
                   ${OPTIONAL_ARGS})

    BUILD_DEP_TARGET()

    # Link libraries for target interface
    list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES LIBRARY::Brotli
                                                                    LIBRARY::NGHttp2
                                                                    OpenSSL::Crypto
                                                                    OpenSSL::SSL
                                                                    LIBRARY::ZLIB)

    # Add dependencies to build target
    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} LIBRARY::Brotli
                                                                        LIBRARY::NGHttp2
                                                                        OpenSSL::SSL
                                                                        OpenSSL::Crypto
                                                                        LIBRARY::ZLIB)
  endmacro()

  # If there is a potential this library can be built internally
  # Check its dependencies to allow forcing this lib to be built if one of its
  # dependencies requires being rebuilt
  if(ENABLE_INTERNAL_CURL)
    # Dependency list of this find module for an INTERNAL build
    set(${CMAKE_FIND_PACKAGE_NAME}_DEPLIST Brotli
                                           NGHttp2
                                           OpenSSL
                                           ZLIB)

    check_dependency_build(${CMAKE_FIND_PACKAGE_NAME} "${${CMAKE_FIND_PACKAGE_NAME}_DEPLIST}")
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC curl)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME CURL)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  find_package(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                                                         HINTS ${DEPENDS_PATH}/lib/cmake
                                                         ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

  # cmake config may not be available (eg Debian libcurl-dev package)
  # fallback to pkgconfig for non windows platforms
  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})
    # We explicitly skip a pkgconfig search for Darwin platforms, as system zlib can not
    # be found by pkg-config, and a search for Curl's Libs field is made during the
    # pkg_check_modules call
    if(PKG_CONFIG_FOUND AND NOT ((WIN32 OR WINDOWSSTORE) OR 
                                 (CMAKE_SYSTEM_NAME MATCHES "Darwin")))
      pkg_check_modules(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} libcurl${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  # Check for existing Curl. If version >= CURL-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal curl, build anyway
  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_CURL) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_CURL) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    # Maybe need to look explicitly for CURL::libcurl_static/shared?
    if(TARGET CURL::libcurl)
      # CURL::libcurl is an alias. We need to get the actual aias target, as we cant make an
      # alias of an alias (ie our ${APP_NAME_LC}::Curl cant be an alias of Curl::libcurl)
      get_target_property(_CURL_ALIASTARGET CURL::libcurl ALIASED_TARGET)

      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # eg Debian's config file is CURLConfigTargets-none.cmake
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_CURL_CONFIGURATIONS ${_CURL_ALIASTARGET} IMPORTED_CONFIGURATIONS)
      if(_CURL_CONFIGURATIONS)
        foreach(_curl_config IN LISTS _CURL_CONFIGURATIONS)
          # Some non standard config (eg None on Debian)
          # Just set to RELEASE var so select_library_configurations can continue to work its magic
          string(TOUPPER ${_curl_config} _curl_config_UPPER)
          if((NOT ${_curl_config_UPPER} STREQUAL "RELEASE") AND
             (NOT ${_curl_config_UPPER} STREQUAL "DEBUG"))
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE ${_CURL_ALIASTARGET} IMPORTED_LOCATION_${_curl_config_UPPER})
          else()
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_${_curl_config_UPPER} ${_CURL_ALIASTARGET} IMPORTED_LOCATION_${_curl_config_UPPER})
          endif()
        endforeach()
      else()
        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE ${_CURL_ALIASTARGET} IMPORTED_LOCATION)
      endif()

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR CURL::libcurl INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Curl
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(Curl_FOUND)
    # cmake target and not building internal
    if(TARGET CURL::libcurl AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # CURL::libcurl is an alias. We need to get the actual aias target, as we cant make an
      # alias of an alias (ie our ${APP_NAME_LC}::Curl cant be an alias of Curl::libcurl)
      if(NOT _CURL_ALIASTARGET)
        get_target_property(_CURL_ALIASTARGET CURL::libcurl ALIASED_TARGET)
      endif()

      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_CURL_ALIASTARGET})
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS CURL_STATICLIB)
      ADD_TARGET_COMPILE_DEFINITION()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Curl_FIND_REQUIRED)
      message(FATAL_ERROR "Curl libraries were not found.")
    endif()
  endif()
endif()
