# FindOpenSSL
# -----------
# Finds the Openssl libraries
#
# This will define the following target:
#
#   ${APP_NAME_LC}::OpenSSL - Alias of OpenSSL::SSL target
#   LIBRARY::OpenSSL - Alias of OpenSSL::SSL target
#   OpenSSL::SSL - standard Openssl SSL target from system find package
#   OpenSSL::Crypto - standard Openssl Crypto target from system find package

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  # We do this dance to utilise cmake system FindOpenssl. Saves us dealing with it
  set(_temp_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
  unset(CMAKE_MODULE_PATH)

  if(OpenSSL_FIND_REQUIRED)
    set(REQ "REQUIRED")
  endif()

  # Only aim for static libs on windows or depends builds
  if(KODI_DEPENDSBUILD OR (WIN32 OR WINDOWS_STORE))
    set(OPENSSL_USE_STATIC_LIBS ON)
    set(OPENSSL_ROOT_DIR ${DEPENDS_PATH})
  endif()

  find_package(OpenSSL ${REQ} ${SEARCH_QUIET})
  unset(OPENSSL_USE_STATIC_LIBS)

  # Back to our normal module paths
  set(CMAKE_MODULE_PATH ${_temp_CMAKE_MODULE_PATH})

  if(OPENSSL_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS OpenSSL::SSL)
    add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS OpenSSL::SSL)

    # Add Crypto as a link library to easily propagate both targets to our custom target
    set_target_properties(OpenSSL::SSL PROPERTIES
                                       INTERFACE_LINK_LIBRARIES "OpenSSL::Crypto")

    if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
      set_target_properties(OpenSSL::SSL PROPERTIES
                                         INTERFACE_LINK_OPTIONS "-Wl,--exclude-libs,$<TARGET_FILE_NAME:OpenSSL::SSL>")

      set_target_properties(OpenSSL::Crypto PROPERTIES
                                            INTERFACE_LINK_OPTIONS "-Wl,--exclude-libs,$<TARGET_FILE_NAME:OpenSSL::Crypto>")
    endif()

    # Required for external searches. Not used internally
    set(OpenSSL_FOUND ON CACHE BOOL "OpenSSL found")
    mark_as_advanced(OpenSSL_FOUND)
  else()
    if(OpenSSL_FIND_REQUIRED)
      message(FATAL_ERROR "OpenSSL libraries were not found.")
    endif()
  endif()
endif()
