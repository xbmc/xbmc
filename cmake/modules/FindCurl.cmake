#.rst:
# FindCurl
# --------
# Finds the Curl library
#
# This will define the following variables::
#
# CURL_FOUND - system has Curl
# CURL_INCLUDE_DIRS - the Curl include directory
# CURL_LIBRARIES - the Curl libraries
# CURL_DEFINITIONS - the Curl definitions
#
# and the following imported targets::
#
#   Curl::Curl   - The Curl library

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CURL libcurl QUIET)
endif()

find_path(CURL_INCLUDE_DIR NAMES curl/curl.h
                           HINTS ${PC_CURL_INCLUDEDIR})
find_library(CURL_LIBRARY NAMES curl libcurl libcurl_imp
                          HINTS ${PC_CURL_LIBDIR})

set(CURL_VERSION ${PC_CURL_VERSION})

set(CURL_LIB_TYPE SHARED)
set(CURL_LDFLAGS ${PC_CURL_LDFLAGS})

# check if curl is statically linked
if(${CURL_LIBRARY} MATCHES ".+\.a$" AND PC_CURL_STATIC_LDFLAGS)
  set(CURL_LIB_TYPE STATIC)
  set(CURL_LDFLAGS ${PC_CURL_STATIC_LDFLAGS})

  pkg_check_modules(PC_NGHTTP2 libnghttp2 QUIET)
  find_library(NGHTTP2_LIBRARY NAMES libnghttp2 nghttp2
                               HINTS ${PC_NGHTTP2_LIBDIR})

  pkg_check_modules(PC_BROTLIDEC libbrotlidec QUIET)
  find_library(BROTLIDEC_LIBRARY NAMES brotlidec
                                 HINTS ${PC_BROTLIDEC_LIBDIR})

  pkg_check_modules(PC_BROTLICOMMON libbrotlicommon QUIET)
  find_library(BROTLICOMMON_LIBRARY NAMES brotlicommon
                                    HINTS ${PC_BROTLICOMMON_LIBDIR})
  set(BROTLI_LIBRARIES ${BROTLIDEC_LIBRARY} ${BROTLICOMMON_LIBRARY})

endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Curl
                                  REQUIRED_VARS CURL_LIBRARY CURL_INCLUDE_DIR
                                  VERSION_VAR CURL_VERSION)

if(CURL_FOUND)
  set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
  set(CURL_LIBRARIES ${CURL_LIBRARY} ${NGHTTP2_LIBRARY} ${BROTLI_LIBRARIES})

  if(NOT TARGET Curl::Curl)
    add_library(Curl::Curl ${CURL_LIB_TYPE} IMPORTED)
    set_target_properties(Curl::Curl PROPERTIES
                                     IMPORTED_LOCATION "${CURL_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIR}")
    if(HAS_CURL_STATIC)
        set_target_properties(Curl::Curl PROPERTIES
                                         INTERFACE_COMPILE_DEFINITIONS HAS_CURL_STATIC=1)
    endif()
  endif()
endif()

mark_as_advanced(CURL_INCLUDE_DIR CURL_LIBRARY CURL_LDFLAGS)
