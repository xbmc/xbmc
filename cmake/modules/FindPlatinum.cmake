#.rst:
# FindPlatinum
# --------
# Finds the Platinum library
#
# This will define the following variables::
#
# PLATINUM_FOUND - system has Platinum
# PLATINUM_INCLUDE_DIRS - the Platinum include directories
# PLATINUM_LIBRARIES - the Platinum libraries
# PLATINUM_DEFINITIONS - the platinum definitions

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PLATINUM platinum QUIET)
endif()

find_library(PLATINUM_LIBRARY NAMES Platinum libPlatinum
                              PATHS ${PC_PLATINUM_LIBDIR})

find_path(PLATINUM_INCLUDE_DIR NAMES Platinum.h
                               PATHS ${PC_PLATINUM_INCLUDEDIR}
                               PATH_SUFFIXES Platinum)

set(PLATINUM_VERSION ${PC_PLATINUM_VERSION})

find_package(Neptune)

if(ENABLE_INTERNAL_PLATINUM)
  include(ExternalProject)

  # Extract version
  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/platinum/Makefile VER)
  string(REGEX MATCH "VERSION=([^ ;]*)" PLATINUM_VER "${VER}")
  set(PLATINUM_VER ${CMAKE_MATCH_1})

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(PLATINUM_URL)
    get_filename_component(PLATINUM_URL "${PLATINUM_URL}" ABSOLUTE)
  else()
    set(PLATINUM_URL https://github.com/lrusak/Platinum/archive/${PLATINUM_VER}.tar.gz)
  endif()

  if(VERBOSE)
    message(STATUS "PLATINUM_URL: ${PLATINUM_URL}")
  endif()

  set(PLATINUM_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libPlatinum.a)
  set(PLATINUM_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include/Platinum)
  set(PLATINUM_VERSION ${PLATINUM_VER})

  externalproject_add(platinum
                      URL ${PLATINUM_URL}
                      DOWNLOAD_NAME Platinum-${PLATINUM_VER}.tar.gz
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/platinum
                      CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR} -DCMAKE_MODULE_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/cmake/Neptune -DCMAKE_INCLUDE_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include/Neptune
                      BUILD_BYPRODUCTS ${PLATINUM_LIBRARY})

  set_target_properties(platinum PROPERTIES FOLDER "External Projects")
  add_dependencies(platinum neptune)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Platinum
                                  REQUIRED_VARS PLATINUM_LIBRARY PLATINUM_INCLUDE_DIR
                                  VERSION_VAR PLATINUM_VERSION)

if(PLATINUM_FOUND)
  set(PLATINUM_INCLUDE_DIRS ${PLATINUM_INCLUDE_DIR} ${NEPTUNE_INCLUDE_DIRS})
  set(PLATINUM_LIBRARIES ${PLATINUM_LIBRARY} ${NEPTUNE_LIBRARIES})
  set(PLATINUM_DEFINITIONS -DHAS_UPNP=1)
endif()

mark_as_advanced(PLATINUM_INCLUDE_DIR PLATINUM_LIBRARY)
