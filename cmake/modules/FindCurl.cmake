#.rst:
# FindCurl
# --------
# Finds the Curl library
#
# This will define the following target:
#
#   Curl::Curl   - The Curl library
#
# Note: Curl has the beginnings of native cmake config support, but its not complete
#       as yet apparently. Look to leverage that for UNIX platforms in the future possibly
#

if(NOT TARGET CURL::libcurl)
  if(Curl_FIND_REQUIRED)
    set(_find_required "REQUIRED")
  endif()
  if(WIN32 OR WINDOWS_STORE)
    include(FindPackageMessage)

    find_package(CURL CONFIG ${_find_required})

    if(CURL_FOUND)
      # Specifically tailored to kodi windows cmake config - Prebuilt as RelWithDebInfo always currently
      get_target_property(CURL_LIB CURL::libcurl IMPORTED_LOCATION_RELWITHDEBINFO)
      get_target_property(CURL_INCLUDE_DIR CURL::libcurl INTERFACE_INCLUDE_DIRECTORIES)
      find_package_message(CURL "Found CURL: ${CURL_LIB}" "[${CURL_LIB}][${CURL_INCLUDE_DIR}]")
    endif()
  else()
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(CURL libcurl ${_find_required} IMPORTED_TARGET GLOBAL)
    else()
      find_path(CURL_INCLUDE_DIR NAMES curl/curl.h
                                 PATHS ${PC_CURL_INCLUDEDIR})
      find_library(CURL_LIBRARY NAMES curl libcurl libcurl_imp
                                PATHS ${PC_CURL_LIBDIR})

      include(FindPackageHandleStandardArgs)
      find_package_handle_standard_args(Curl
                                        REQUIRED_VARS CURL_LIBRARY CURL_INCLUDE_DIR
                                        VERSION_VAR CURL_VERSION)
    endif()
  endif()

  if(CURL_FOUND)
    if(TARGET PkgConfig::CURL)
      add_library(CURL::libcurl ALIAS PkgConfig::CURL)
    elseif(NOT TARGET CURL::libcurl)
      add_library(CURL::libcurl UNKNOWN IMPORTED)
      set_target_properties(CURL::libcurl PROPERTIES
                                          IMPORTED_LOCATION "${CURL_LIBRARY}"
                                          INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIR}")
    endif()
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP CURL::libcurl)
  endif()

  mark_as_advanced(CURL_INCLUDE_DIR CURL_LIBRARY CURL_LDFLAGS)
endif()
