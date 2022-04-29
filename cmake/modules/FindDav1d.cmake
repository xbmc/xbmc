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
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC dav1d)

  SETUP_BUILD_VARS()

  set(DAV1D_VERSION ${${MODULE}_VER})

  find_program(NINJA_EXECUTABLE ninja REQUIRED)
  find_program(MESON_EXECUTABLE meson REQUIRED)

  set(CONFIGURE_COMMAND ${MESON_EXECUTABLE}
                        --buildtype=release
                        --default-library=static
                        --prefix=${DEPENDS_PATH}
                        --libdir=lib
                        -Denable_asm=true
                        -Denable_tools=false
                        -Denable_examples=false
                        -Denable_tests=false
                        ../dav1d)
  set(BUILD_COMMAND ${NINJA_EXECUTABLE})
  set(INSTALL_COMMAND ${NINJA_EXECUTABLE} install)

  BUILD_DEP_TARGET()
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
