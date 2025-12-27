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
  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_CURL) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_CURL) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    # cmake target and not building internal
    if(TARGET CURL::libcurl AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # CURL::libcurl is an alias. We need to get the actual aias target, as we cant make an
      # alias of an alias (ie our ${APP_NAME_LC}::Curl cant be an alias of Curl::libcurl)
      get_target_property(_CURL_ALIASTARGET CURL::libcurl ALIASED_TARGET)

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
