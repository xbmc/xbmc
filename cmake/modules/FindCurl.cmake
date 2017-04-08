#.rst:
# FindCurl
# --------
# Finds the Curl library
#
# This will will define the following variables::
#
# CURL_FOUND - system has Curl
# CURL_INCLUDE_DIRS - the Curl include directory
# CURL_LIBRARIES - the Curl libraries
# CURL_DEFINITIONS - the Curl definitions
#
# and the following imported targets::
#
#   Curl::Curl   - The Curl library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CURL libcurl QUIET)
endif()

find_path(CURL_INCLUDE_DIR NAMES curl/curl.h
                           PATHS ${PC_CURL_INCLUDEDIR})
find_library(CURL_LIBRARY NAMES curl libcurl
                          PATHS ${PC_CURL_LIBDIR})

set(CURL_VERSION ${PC_CURL_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Curl
                                  REQUIRED_VARS CURL_LIBRARY CURL_INCLUDE_DIR
                                  VERSION_VAR CURL_VERSION)

if(CURL_FOUND)
  set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
  set(CURL_LIBRARIES ${CURL_LIBRARY})

  # Check whether OpenSSL inside libcurl is static.
  if(UNIX)
    if(NOT DEFINED HAS_CURL_STATIC)
      get_filename_component(CURL_LIBRARY_DIR ${CURL_LIBRARY} DIRECTORY)
      find_soname(CURL REQUIRED)

      if(APPLE)
        set(libchecker nm)
        set(searchpattern "T [_]?CRYPTO_set_locking_call")
      else()
        set(libchecker readelf -s)
        set(searchpattern "CRYPTO_set_locking_call")
      endif()
      execute_process(
        COMMAND ${libchecker} ${CURL_LIBRARY_DIR}/${CURL_SONAME}
        COMMAND grep -Eq ${searchpattern}
        RESULT_VARIABLE HAS_CURL_STATIC)
      unset(libchecker)
      unset(searchpattern)
      if(HAS_CURL_STATIC EQUAL 0)
        set(HAS_CURL_STATIC TRUE)
      else()
        set(HAS_CURL_STATIC FALSE)
      endif()
      set(HAS_CURL_STATIC ${HAS_CURL_STATIC} CACHE INTERNAL
          "OpenSSL is statically linked into Curl")
      message(STATUS "OpenSSL is statically linked into Curl: ${HAS_CURL_STATIC}")
    endif()
  endif()

  if(HAS_CURL_STATIC)
    set(CURL_DEFINITIONS -DHAS_CURL_STATIC=1)
  endif()

  if(NOT TARGET Curl::Curl)
    add_library(Curl::Curl UNKNOWN IMPORTED)
    set_target_properties(Curl::Curl PROPERTIES
                                     IMPORTED_LOCATION "${CURL_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIR}")
    if(HAS_CURL_STATIC)
        set_target_properties(Curl::Curl PROPERTIES
                                         INTERFACE_COMPILE_DEFINITIONS HAS_CURL_STATIC=1)
    endif()
  endif()
endif()

mark_as_advanced(CURL_INCLUDE_DIR CURL_LIBRARY)
