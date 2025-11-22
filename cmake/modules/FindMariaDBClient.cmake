#.rst:
# FindMariaDBClient
# ---------------
# Finds the MariaDBClient library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::MariaDBClient   - The mariadb-c-connector library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroMariaDBClient)

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB TRUE)
    endif()

    if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB)
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
    endif()

    if(WIN32 OR WINDOWS_STORE)
      set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/03-win-uwp.patch"
                  "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/04-win-installpkgconfig.patch")

      list(APPEND BUILD_FLAGS -DBUILD_SHARED_LIBS:BOOL=ON
                              -DINSTALL_SHARED=ON)
      list(APPEND CLIENT_PLUGINS -DPLUGIN_PVIO_NPIPE:STRING=STATIC
                                 -DPLUGIN_PVIO_SHMEM:STRING=STATIC)

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB_PATH lib/mariadb)
    else()
      if("${CORE_SYSTEM_NAME}" STREQUAL "android")
        set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-android.patch")
      elseif("${CORE_SYSTEM_NAME}" STREQUAL "linux")
        set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/02-linux-pthread.patch")
      endif()

      list(APPEND BUILD_FLAGS -DWITH_MYSQLCOMPAT:BOOL=OFF
                              -DINSTALL_STATIC=ON)

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH lib/mariadb)
    endif()

    list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/05-all-installtargets.patch")

    generate_patchcommand("${patches}")
    unset(patches)

    if(WINDOWS_STORE)
      list(APPEND CLIENT_PLUGINS -DCLIENT_PLUGIN_DIALOG=OFF)
    else()
      list(APPEND CLIENT_PLUGINS -DCLIENT_PLUGIN_DIALOG=STATIC)
    endif()

    # build plugins as static for all platforms
    list(APPEND CLIENT_PLUGINS -DCLIENT_PLUGIN_SHA256_PASSWORD=STATIC
                               -DCLIENT_PLUGIN_CACHING_SHA2_PASSWORD=STATIC
                               -DCLIENT_PLUGIN_MYSQL_CLEAR_PASSWORD=STATIC
                               -DCLIENT_PLUGIN_MYSQL_OLD_PASSWORD=STATIC
                               -DCLIENT_PLUGIN_CLIENT_ED25519=STATIC)

    # Disable GSSAPI authentication plugin (not widely used by Kodi users)
    list(APPEND CLIENT_PLUGINS -DCLIENT_PLUGIN_AUTH_GSSAPI_CLIENT=OFF)

    # tvos: warning: setcontext is not implemented and will always fail
    if("${CORE_PLATFORM_NAME_LC}" STREQUAL "tvos")
      list(APPEND BUILD_FLAGS -DHAVE_UCONTEXT_H=
                              -DHAVE_FILE_UCONTEXT_H=)
    endif()

    # webos: warning: setcontext is not implemented and will always fail
    if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
      list(APPEND BUILD_FLAGS -DHAVE_UCONTEXT_H=
                              -DHAVE_FILE_UCONTEXT_H=)
    endif()

    set(CMAKE_ARGS -DWITH_SSL=OPENSSL
                   -DWITH_UNIT_TESTS:BOOL=OFF
                   -DWITH_EXTERNAL_ZLIB:BOOL=ON
                   -DWITH_CURL:BOOL=OFF
                   -DCMAKE_COMPILE_WARNING_AS_ERROR=OFF
                   ${CLIENT_PLUGINS}
                   ${BUILD_FLAGS})

    BUILD_DEP_TARGET()

    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} LIBRARY::OpenSSL
                                                                        LIBRARY::ZLIB)

    if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES LIBRARY::OpenSSL
                                                                      LIBRARY::ZLIB)
    endif()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC mariadb)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME_PC libmariadb)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_MARIADBCLIENT) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_MARIADBCLIENT) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_MARIADB)
    ADD_TARGET_COMPILE_DEFINITION()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(MariaDBClient_FIND_REQUIRED)
      message(FATAL_ERROR "Mariadb-c-connector libraries were not found.")
    endif()
  endif()
endif()
