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

  macro(buildCurl)
    find_package(Brotli REQUIRED QUIET)
    find_package(NGHttp2 REQUIRED QUIET)
    find_package(OpenSSL REQUIRED QUIET)

    # Darwin platforms link against toolchain provided zlib regardless
    # They will fail when searching for static. All other platforms, prefer static
    # if possible (requires cmake 3.24+ otherwise variable is a no-op)
    # Windows still uses dynamic lib for zlib for other purposes, dont mix
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" AND NOT (WIN32 OR WINDOWS_STORE))
      set(ZLIB_USE_STATIC_LIBS ON)
    endif()
    find_package(Zlib REQUIRED)
    unset(ZLIB_USE_STATIC_LIBS)

    set(CURL_VERSION ${${MODULE}_VER})
    # Curl debug uses postfix -d for all platforms
    set(CURL_DEBUG_POSTFIX -d)

    if(WIN32 OR WINDOWS_STORE)
      set(CURL_C_FLAGS -DNGHTTP2_STATICLIB)
      set(PLATFORM_LINK_LIBS crypt32.lib)
    endif()

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/01-win-nghttp2-add-name.patch")

    generate_patchcommand("${patches}")

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
    set(PC_CURL_LINK_LIBRARIES Brotli::Brotli NGHttp2::NGHttp2 OpenSSL::Crypto OpenSSL::SSL ZLIB::ZLIB ${PLATFORM_LINK_LIBS})

    # Add dependencies to build target
    add_dependencies(${MODULE_LC} Brotli::Brotli)
    add_dependencies(${MODULE_LC} NGHttp2::NGHttp2)
    add_dependencies(${MODULE_LC} OpenSSL::SSL)
    add_dependencies(${MODULE_LC} OpenSSL::Crypto)
    add_dependencies(${MODULE_LC} ZLIB::ZLIB)
  endmacro()

  set(MODULE_LC curl)

  SETUP_BUILD_VARS()

  find_package(CURL CONFIG QUIET
                    HINTS ${DEPENDS_PATH}/lib/cmake
                    ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # Check for existing Curl. If version >= CURL-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal curl, build anyway
  if((CURL_VERSION VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_CURL) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_CURL))

    buildCurl()
  else()
    # Maybe need to look explicitly for CURL::libcurl_static/shared?
    if(NOT TARGET CURL::libcurl)
      find_package(PkgConfig)

      # We only rely on pkgconfig for non windows platforms
      if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
        pkg_check_modules(CURL libcurl QUIET)

        # First item is the full path of the library file found
        # pkg_check_modules does not populate a variable of the found library explicitly
        list(GET CURL_LINK_LIBRARIES 0 CURL_LIBRARY_RELEASE)

        # Add link libraries for static lib usage
        if(${CURL_LIBRARY} MATCHES ".+\.a$" AND CURL_LINK_LIBRARIES)
          # Remove duplicates
          list(REMOVE_DUPLICATES CURL_LINK_LIBRARIES)

          # Remove own library - eg libcurl.a
          list(FILTER CURL_LINK_LIBRARIES EXCLUDE REGEX ".*curl.*\.a$")
          set(PC_CURL_LINK_LIBRARIES ${CURL_LINK_LIBRARIES})
        endif()

        # pkgconfig sets CURL_INCLUDEDIR, map this to our "standard" variable name
        set(CURL_INCLUDE_DIR ${CURL_INCLUDEDIR})
      else()
        find_path(CURL_INCLUDE_DIR NAMES curl/curl.h
                                   HINTS ${DEPENDS_PATH}/include
                                   ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
        find_library(CURL_LIBRARY_RELEASE NAMES curl libcurl libcurl_imp
                                          HINTS ${DEPENDS_PATH}/lib
                                          ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
      endif()
    else()
      # CURL::libcurl is an alias. We need to get the actual aias target, as we cant make an
      # alias of an alias (ie our ${APP_NAME_LC}::Curl cant be an alias of Curl::libcurl)
      get_target_property(_CURL_ALIASTARGET CURL::libcurl ALIASED_TARGET)

      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # eg Debian's config file is CURLConfigTargets-none.cmake
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_CURL_CONFIGURATIONS ${_CURL_ALIASTARGET} IMPORTED_CONFIGURATIONS)
      foreach(_curl_config IN LISTS _CURL_CONFIGURATIONS)
        # Some non standard config (eg None on Debian)
        # Just set to RELEASE var so select_library_configurations can continue to work its magic
        string(TOUPPER ${_curl_config} _curl_config_UPPER)
        if((NOT ${_curl_config_UPPER} STREQUAL "RELEASE") AND
           (NOT ${_curl_config_UPPER} STREQUAL "DEBUG"))
          get_target_property(CURL_LIBRARY_RELEASE ${_CURL_ALIASTARGET} IMPORTED_LOCATION_${_curl_config_UPPER})
        else()
          get_target_property(CURL_LIBRARY_${_curl_config_UPPER} ${_CURL_ALIASTARGET} IMPORTED_LOCATION_${_curl_config_UPPER})
        endif()
      endforeach()

      get_target_property(CURL_INCLUDE_DIR CURL::libcurl INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(CURL)
  unset(CURL_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Curl
                                    REQUIRED_VARS CURL_LIBRARY CURL_INCLUDE_DIR
                                    VERSION_VAR CURL_VERSION)

  if(CURL_FOUND)
    # cmake target and not building internal
    if(TARGET CURL::libcurl AND NOT TARGET curl)
      # CURL::libcurl is an alias. We need to get the actual aias target, as we cant make an
      # alias of an alias (ie our ${APP_NAME_LC}::Curl cant be an alias of Curl::libcurl)
      if(NOT _CURL_ALIASTARGET)
        get_target_property(_CURL_ALIASTARGET CURL::libcurl ALIASED_TARGET)
      endif()

      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_CURL_ALIASTARGET})
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIR}")

      if(CURL_LIBRARY_RELEASE)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                                         IMPORTED_LOCATION_RELEASE "${CURL_LIBRARY_RELEASE}")
      endif()
      if(CURL_LIBRARY_DEBUG)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_LOCATION_DEBUG "${CURL_LIBRARY_DEBUG}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              IMPORTED_CONFIGURATIONS DEBUG)
      endif()

      # Add link libraries for static lib usage found from pkg-config
      if(PC_CURL_LINK_LIBRARIES)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         INTERFACE_LINK_LIBRARIES "${PC_CURL_LINK_LIBRARIES}")
      endif()

      if(WIN32 OR WINDOWS_STORE)
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS "CURL_STATICLIB")
      endif()

    endif()

    if(TARGET curl)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} curl)
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
      if(NOT TARGET curl)
        buildCurl()
        set_target_properties(curl PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends curl)
    endif()
  else()
    if(Curl_FIND_REQUIRED)
      message(FATAL_ERROR "Curl libraries were not found.")
    endif()
  endif()
endif()
