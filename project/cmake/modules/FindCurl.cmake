# - Try to find CURL
# Once done this will define
#
# CURL_FOUND - system has libcurl
# CURL_INCLUDE_DIRS - the libcurl include directory
# CURL_LIBRARIES - The libcurl libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (CURL libcurl)
  list(APPEND CURL_INCLUDE_DIRS ${CURL_INCLUDEDIR})
else()
  find_path(CURL_INCLUDE_DIRS curl/curl.h)
  find_library(CURL_LIBRARIES NAMES curl libcurl)
endif()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Curl DEFAULT_MSG CURL_INCLUDE_DIRS CURL_LIBRARIES)

mark_as_advanced(CURL_INCLUDE_DIRS CURL_LIBRARIES)

if(CURL_FOUND)
  if(NOT CURL_LIBRARY_DIRS AND CURL_LIBDIR)
    set(CURL_LIBRARY_DIRS ${CURL_LIBDIR})
  endif()

  find_soname(CURL)

  if(EXISTS "${CURL_LIBRARY_DIRS}/${CURL_SONAME}")
    execute_process(COMMAND readelf -s ${CURL_LIBRARY_DIRS}/${CURL_SONAME} COMMAND grep CRYPTO_set_locking_call OUTPUT_VARIABLE HAS_CURL_STATIC)
  else()
    message(FATAL_ERROR "curl library not found")
  endif()
endif()

if(HAS_CURL_STATIC)
  mark_as_advanced(HAS_CURL_STATIC)
  list(APPEND CURL_DEFINITIONS -DHAS_CURL_STATIC=1)
endif()
