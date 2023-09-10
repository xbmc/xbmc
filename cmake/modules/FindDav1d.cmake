#.rst:
# FindDav1d
# --------
# Finds the dav1d library
#
# This will define the following target:
#
#   dav1d::dav1d   - The dav1d library

if(NOT TARGET dav1d::dav1d)
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
                               PATHS ${PC_DAV1D_LIBDIR}
                               NO_CACHE)

    find_path(DAV1D_INCLUDE_DIR NAMES dav1d/dav1d.h
                                PATHS ${PC_DAV1D_INCLUDEDIR}
                                NO_CACHE)

    set(DAV1D_VERSION ${PC_DAV1D_VERSION})
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Dav1d
                                    REQUIRED_VARS DAV1D_LIBRARY DAV1D_INCLUDE_DIR
                                    VERSION_VAR DAV1D_VERSION)

  if(DAV1D_FOUND)
    add_library(dav1d::dav1d UNKNOWN IMPORTED)
    set_target_properties(dav1d::dav1d PROPERTIES
                                             IMPORTED_LOCATION "${DAV1D_LIBRARY}"
                                             INTERFACE_INCLUDE_DIRECTORIES "${DAV1D_INCLUDE_DIR}")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP dav1d::dav1d)

    if(TARGET dav1d)
      add_dependencies(dav1d::dav1d dav1d)
    endif()
  endif()
endif()
