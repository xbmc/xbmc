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

if(ENABLE_INTERNAL_DAV1D)
  include(ExternalProject)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC dav1d)

  SETUP_BUILD_VARS()

  set(DAV1D_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libdav1d.a)
  set(DAV1D_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  set(DAV1D_VERSION ${DAV1D_VER})

  externalproject_add(${MODULE_LC}
                      URL ${${MODULE}_URL}
                      URL_HASH ${${MODULE}_HASH}
                      DOWNLOAD_NAME ${${MODULE}_ARCHIVE}
                      DOWNLOAD_DIR ${TARBALL_DIR}
                      PREFIX ${CORE_BUILD_DIR}/${MODULE_LC}
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

  set_target_properties(${MODULE_LC} PROPERTIES FOLDER "External Projects")
else()
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_DAV1D dav1d QUIET)
  endif()

  find_library(DAV1D_LIBRARY NAMES dav1d libdav1d
                             PATHS ${PC_DAV1D_LIBDIR})

  find_path(DAV1D_INCLUDE_DIR NAMES dav1d/dav1d.h
                              PATHS ${PC_DAV1D_INCLUDEDIR})

  set(DAV1D_VERSION ${PC_DAV1D_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Dav1d
                                  REQUIRED_VARS DAV1D_LIBRARY DAV1D_INCLUDE_DIR
                                  VERSION_VAR DAV1D_VERSION)

if(DAV1D_FOUND)
  set(DAV1D_INCLUDE_DIRS ${DAV1D_INCLUDE_DIR})
  set(DAV1D_LIBRARIES ${DAV1D_LIBRARY})
endif()

mark_as_advanced(DAV1D_INCLUDE_DIR DAV1D_LIBRARY)
