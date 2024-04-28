#.rst:
# FindCurl
# --------
# Finds the Curl library
#
# This will define the following target:
#
#   Curl::Curl   - The Curl library

if(NOT TARGET Curl::Curl)
  find_package(PkgConfig)

  # We only rely on pkgconfig for non windows platforms
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(CURL libcurl QUIET)

    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET CURL_LINK_LIBRARIES 0 CURL_LIBRARY)
  else()

    find_path(CURL_INCLUDEDIR NAMES curl/curl.h
                               HINTS ${DEPENDS_PATH}/include
                               ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                               NO_CACHE)
    find_library(CURL_LIBRARY NAMES curl libcurl libcurl_imp
                              HINTS ${DEPENDS_PATH}/lib
                              ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                              NO_CACHE)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Curl
                                    REQUIRED_VARS CURL_LIBRARY CURL_INCLUDEDIR
                                    VERSION_VAR CURL_VERSION)

  if(CURL_FOUND)
    if(NOT TARGET Curl::Curl)
      add_library(Curl::Curl UNKNOWN IMPORTED)
      set_target_properties(Curl::Curl PROPERTIES
                                       IMPORTED_LOCATION "${CURL_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDEDIR}")

      # Add link libraries for static lib usage
      if(${CURL_LIBRARY} MATCHES ".+\.a$" AND CURL_LINK_LIBRARIES)
        # Remove duplicates
        list(REMOVE_DUPLICATES CURL_LINK_LIBRARIES)

        # Remove own library - eg libcurl.a
        list(FILTER CURL_LINK_LIBRARIES EXCLUDE REGEX ".*curl.*\.a$")

        set_target_properties(Curl::Curl PROPERTIES
                                         INTERFACE_LINK_LIBRARIES "${CURL_LINK_LIBRARIES}")
      endif()

      set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Curl::Curl)
    endif()
  else()
    if(Curl_FIND_REQUIRED)
      message(FATAL_ERROR "Curl libraries were not found.")
    endif()
  endif()
endif()
