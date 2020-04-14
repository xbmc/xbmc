#.rst:
# FindNeptune
# --------
# Finds the Neptune library
#
# This will define the following variables::
#
# NEPTUNE_FOUND - system has Neptune
# NEPTUNE_INCLUDE_DIRS - the Neptune include directories
# NEPTUNE_LIBRARIES - the Neptune libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_NEPTUNE neptune QUIET)
endif()

find_library(NEPTUNE_LIBRARY NAMES Neptune libNeptune
                             PATHS ${PC_NEPTUNE_LIBDIR})

find_path(NEPTUNE_INCLUDE_DIR NAMES Neptune.h
                              PATHS ${PC_NEPTUNE_INCLUDEDIR}
                              PATH_SUFFIXES Neptune)

set(NEPTUNE_VERSION ${PC_NEPTUNE_VERSION})

if(ENABLE_INTERNAL_NEPTUNE)
  include(ExternalProject)

  # Extract version
  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/neptune/Makefile VER)
  string(REGEX MATCH "VERSION=([^ ;]*)" NEPTUNE_VER "${VER}")
  set(NEPTUNE_VER ${CMAKE_MATCH_1})

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(NEPTUNE_URL)
    get_filename_component(NEPTUNE_URL "${NEPTUNE_URL}" ABSOLUTE)
  else()
    set(NEPTUNE_URL https://github.com/lrusak/Neptune/archive/${NEPTUNE_VER}.tar.gz)
  endif()

  if(VERBOSE)
    message(STATUS "NEPTUNE_URL: ${NEPTUNE_URL}")
  endif()

  set(NEPTUNE_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libNeptune.a)
  set(NEPTUNE_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include/Neptune)
  set(NEPTUNE_VERSION ${NEPTUNE_VER})

  externalproject_add(neptune
                      URL ${NEPTUNE_URL}
                      DOWNLOAD_NAME Neptune-${NEPTUNE_VER}.tar.gz
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/neptune
                      CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                      BUILD_BYPRODUCTS ${NEPTUNE_LIBRARY})

  set_target_properties(neptune PROPERTIES FOLDER "External Projects")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Neptune
                                  REQUIRED_VARS NEPTUNE_LIBRARY NEPTUNE_INCLUDE_DIR
                                  VERSION_VAR NEPTUNE_VERSION)

if(NEPTUNE_FOUND)
  set(NEPTUNE_INCLUDE_DIRS ${NEPTUNE_INCLUDE_DIR})
  set(NEPTUNE_LIBRARIES ${NEPTUNE_LIBRARY})
endif()

mark_as_advanced(NEPTUNE_INCLUDE_DIR NEPTUNE_LIBRARY)
