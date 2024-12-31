#.rst:
# FindNGHttp2
# ----------
# Finds the NGHttp2 library
#
# This will define the following target:
#
#   NGHttp2::NGHttp2   - The NGHttp2 library

if(NOT TARGET NGHttp2::NGHttp2)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
    pkg_check_modules(NGHTTP2 libnghttp2 QUIET)

    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET NGHTTP2_LINK_LIBRARIES 0 NGHTTP2_LIBRARY)

    set(NGHTTP2_INCLUDE_DIR ${NGHTTP2_INCLUDEDIR})
  else()

    find_path(NGHTTP2_INCLUDE_DIR NAMES nghttp2/nghttp2.h
                                  HINTS ${DEPENDS_PATH}/include
                                  ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
    find_library(NGHTTP2_LIBRARY NAMES nghttp2 nghttp2_static
                                 HINTS ${DEPENDS_PATH}/lib
                                 ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(NGHttp2
                                    REQUIRED_VARS NGHTTP2_LIBRARY NGHTTP2_INCLUDE_DIR
                                    VERSION_VAR NGHTTP2_VERSION)

  if(NGHTTP2_FOUND)
    add_library(NGHttp2::NGHttp2 UNKNOWN IMPORTED)

    set_target_properties(NGHttp2::NGHttp2 PROPERTIES
                                           IMPORTED_LOCATION "${NGHTTP2_LIBRARY}"
                                           INTERFACE_INCLUDE_DIRECTORIES "${NGHTTP2_INCLUDE_DIR}")

    # Todo: for windows, do a find_package config call to retrieve this from the cmake config
    #       For now just explicitly say its a static build
    if(WIN32 OR WINDOWS_STORE)
      set_property(TARGET NGHttp2::NGHttp2 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS "NGHTTP2_STATICLIB")
    endif()
  else()
    if(NGHttp2_FIND_REQUIRED)
      message(FATAL_ERROR "NGHttp2 libraries were not found.")
    endif()
  endif()
endif()
