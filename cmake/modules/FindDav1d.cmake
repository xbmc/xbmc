#.rst:
# FindDav1d
# --------
# Finds the dav1d library
#
# This will define the following variables::
#
# DAV1D_FOUND - system has dav1d
# DAV1D_INCLUDE_DIRS - the dav1d include directories
# DAV1D_LIBRARIES - the dav1d libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_DAV1D dav1d QUIET)
endif()

find_library(DAV1D_LIBRARY NAMES dav1d
                           PATHS ${PC_DAV1D_LIBDIR})

find_path(DAV1D_INCLUDE_DIR NAMES dav1d/dav1d.h
                            PATHS ${PC_DAV1D_INCLUDEDIR})

set(DAV1D_VERSION ${PC_DAV1D_VERSION})

if (NOT DAV1D_LIBRARY AND NOT DAV1D_INCLUDE_DIR AND NOT DAV1D_VERSION)
  set(ENABLE_INTERNAL_DAV1D ON)
  message(STATUS "libdav1d not found, falling back to internal build")
endif()

if(ENABLE_INTERNAL_DAV1D)
  include(ExternalProject)

  # Extract version
  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/dav1d/DAV1D-VERSION VER)

  string(REGEX MATCH "VERSION=[^ ]*$.*" DAV1D_VER "${VER}")
  list(GET DAV1D_VER 0 DAV1D_VER)
  string(SUBSTRING "${DAV1D_VER}" 8 -1 DAV1D_VER)

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(DAV1D_URL)
    get_filename_component(DAV1D_URL "${DAV1D_URL}" ABSOLUTE)
  else()
    set(DAV1D_URL http://mirrors.kodi.tv/build-deps/sources/dav1d-${DAV1D_VER}.tar.gz)
  endif()

  if(VERBOSE)
    message(STATUS "DAV1D_URL: ${DAV1D_URL}")
  endif()

  set(DAV1D_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libdav1d.a)
  set(DAV1D_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  set(DAV1D_VERSION ${DAV1D_VER})

  externalproject_add(dav1d
                      URL ${DAV1D_URL}
                      DOWNLOAD_NAME dav1d-${DAV1D_VER}.tar.gz
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/dav1d
                      CONFIGURE_COMMAND meson
                                        --buildtype=release
                                        --default-library=static
                                        --prefix=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                        --libdir=lib
                                        -Denable_asm=true
                                        -Denable_tools=false
                                        -Denable_examples=false
                                        -Denable_tests=false
                                        ../dav1d
                      BUILD_COMMAND ninja
                      INSTALL_COMMAND ninja install
                      BUILD_BYPRODUCTS ${DAV1D_LIBRARY})

  set_target_properties(dav1d PROPERTIES FOLDER "External Projects")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Dav1d
                                  REQUIRED_VARS DAV1D_LIBRARY DAV1D_INCLUDE_DIR
                                  VERSION_VAR DAV1D_VERSION)

if(DAV1D_FOUND)
  set(DAV1D_INCLUDE_DIRS ${DAVID_INCLUDE_DIR})
  set(DAV1D_LIBRARIES ${DAV1D_LIBRARY})
endif()

mark_as_advanced(DAVID_INCLUDE_DIR DAV1D_LIBRARY)
