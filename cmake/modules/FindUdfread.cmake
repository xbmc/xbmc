#.rst:
# FindUdfread
# --------
# Finds the udfread library
#
# This will define the following variables::
#
# UDFREAD_FOUND - system has udfread
# UDFREAD_INCLUDE_DIRS - the udfread include directory
# UDFREAD_LIBRARIES - the udfread libraries
# UDFREAD_DEFINITIONS - the udfread definitions

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_UDFREAD udfread>=1.0.0 QUIET)
endif()

find_path(UDFREAD_INCLUDE_DIR NAMES udfread/udfread.h
                          PATHS ${PC_UDFREAD_INCLUDEDIR})

find_library(UDFREAD_LIBRARY NAMES udfread libudfread
                         PATHS ${PC_UDFREAD_LIBDIR})

set(UDFREAD_VERSION ${PC_UDFREAD_VERSION})

if(ENABLE_INTERNAL_UDFREAD)
  include(${WITH_KODI_DEPENDS}/packages/libudfread/package.cmake)
  set(UDFREAD_VERSION ${PKG_VERSION})
  add_depends_for_targets("HOST")

  add_custom_target(udfread ALL DEPENDS libudfread-host)

  set(UDFREAD_LIBRARY ${INSTALL_PREFIX_HOST}/lib/libudfread.a)
  set(UDFREAD_INCLUDE_DIR ${INSTALL_PREFIX_HOST}/include)

  set_target_properties(udfread PROPERTIES FOLDER "External Projects")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Udfread
                                  REQUIRED_VARS UDFREAD_LIBRARY UDFREAD_INCLUDE_DIR
                                  VERSION_VAR UDFREAD_VERSION)

if(UDFREAD_FOUND)
  set(UDFREAD_LIBRARIES ${UDFREAD_LIBRARY})
  set(UDFREAD_INCLUDE_DIRS ${UDFREAD_INCLUDE_DIR})
  set(UDFREAD_DEFINITIONS -DHAS_UDFREAD=1)
endif()

mark_as_advanced(UDFREAD_INCLUDE_DIR UDFREAD_LIBRARY)
