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

find_library(DAV1D_LIBRARY NAMES dav1d libdav1d
                           PATHS ${PC_DAV1D_LIBDIR})

find_path(DAV1D_INCLUDE_DIR NAMES dav1d/dav1d.h
                            PATHS ${PC_DAV1D_INCLUDEDIR})

set(DAV1D_VERSION ${PC_DAV1D_VERSION})

if(ENABLE_INTERNAL_DAV1D)
  include(${WITH_KODI_DEPENDS}/packages/dav1d/package.cmake)
  add_depends_for_targets("HOST")

  add_custom_target(dav1d ALL DEPENDS dav1d-host)

  set(DAV1D_LIBRARY ${INSTALL_PREFIX_HOST}/lib/libdav1d.a)
  set(DAV1D_INCLUDE_DIR ${INSTALL_PREFIX_HOST}/include)

  set_target_properties(dav1d PROPERTIES FOLDER "External Projects")
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
