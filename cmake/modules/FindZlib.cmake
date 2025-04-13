# FindZLIB
# -----------
# Finds the zlib library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::ZLIB - Alias to ZLIB::ZLIB target
#   LIBRARY::ZLIB - Alias to ZLIB::ZLIB target
#   ZLIB::ZLIB - standard Zlib target from system find package
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  # We do this dance to utilise cmake system FindZLIB. Saves us dealing with it
  set(_temp_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
  unset(CMAKE_MODULE_PATH)

  if(ZLIB_FIND_REQUIRED)
    set(REQ "REQUIRED")
  endif()

  if(KODI_DEPENDSBUILD OR (WIN32 OR WINDOWS_STORE))
    set(ZLIB_ROOT ${DEPENDS_PATH})
  endif()

  # Darwin platforms link against toolchain provided zlib regardless
  # They will fail when searching for static. All other platforms, prefer static
  # if possible (requires cmake 3.24+ otherwise variable is a no-op)
  # Windows still uses dynamic lib for zlib for other purposes, dont mix
  if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" AND NOT (WIN32 OR WINDOWS_STORE))
    set(ZLIB_USE_STATIC_LIBS ON)
  endif()

  find_package(ZLIB ${SEARCH_QUIET} ${REQ})
  unset(ZLIB_USE_STATIC_LIBS)

  # Back to our normal module paths
  set(CMAKE_MODULE_PATH ${_temp_CMAKE_MODULE_PATH})

  if(ZLIB_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS ZLIB::ZLIB)
    add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ZLIB::ZLIB)

    # Required for external searches. Not used internally
    set(ZLIB_FOUND ON CACHE BOOL "ZLIB found")
    mark_as_advanced(ZLIB_FOUND)
  else()
    if(ZLIB_FIND_REQUIRED)
      message(FATAL_ERROR "Zlib libraries were not found.")
    endif()
  endif()
endif()
