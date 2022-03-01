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

if(ENABLE_INTERNAL_UDFREAD)
  include(ExternalProject)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC udfread)

  SETUP_BUILD_VARS()

  set(UDFREAD_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libudfread.a)
  set(UDFREAD_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  set(UDFREAD_VERSION ${${MODULE}_VER})

  externalproject_add(udfread
                      URL ${${MODULE}_URL}
                      URL_HASH ${${MODULE}_HASH}
                      DOWNLOAD_NAME ${${MODULE}_ARCHIVE}
                      DOWNLOAD_DIR ${TARBALL_DIR}
                      PREFIX ${CORE_BUILD_DIR}/${MODULE_LC}
                      CONFIGURE_COMMAND autoreconf -vif &&
                                        ./configure
                                        --enable-static
                                        --disable-shared
                                        --prefix=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                      BUILD_BYPRODUCTS ${UDFREAD_LIBRARY}
                      BUILD_IN_SOURCE 1)

  set_target_properties(udfread PROPERTIES FOLDER "External Projects")
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP udfread)
else()
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_UDFREAD udfread>=1.0.0 QUIET)
  endif()

  find_path(UDFREAD_INCLUDE_DIR NAMES udfread/udfread.h
                            PATHS ${PC_UDFREAD_INCLUDEDIR})

  find_library(UDFREAD_LIBRARY NAMES udfread libudfread
                           PATHS ${PC_UDFREAD_LIBDIR})

  set(UDFREAD_VERSION ${PC_UDFREAD_VERSION})
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
