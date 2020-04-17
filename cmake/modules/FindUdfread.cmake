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
  include(ExternalProject)

  # Extract version
  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/libudfread/UDFREAD-VERSION VER)

  string(REGEX MATCH "VERSION=[^ ]*$.*" UDFREAD_VER "${VER}")
  list(GET UDFREAD_VER 0 UDFREAD_VER)
  string(SUBSTRING "${UDFREAD_VER}" 8 -1 UDFREAD_VER)

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(UDFREAD_URL)
    get_filename_component(UDFREAD_URL "${UDFREAD_URL}" ABSOLUTE)
  else()
    set(UDFREAD_URL http://mirrors.kodi.tv/build-deps/sources/libudfread-${UDFREAD_VER}.tar.gz)
  endif()

  if(VERBOSE)
    message(STATUS "UDFREAD_URL: ${UDFREAD_URL}")
  endif()

  set(UDFREAD_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libudfread.a)
  set(UDFREAD_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  set(UDFREAD_VERSION ${UDFREAD_VER})

  externalproject_add(udfread
                      URL ${UDFREAD_URL}
                      DOWNLOAD_NAME libudfread-${UDFREAD_VER}.tar.gz
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/libudfread
                      CONFIGURE_COMMAND autoreconf -vif &&
                                        ./configure
                                        --enable-static
                                        --disable-shared
                                        --prefix=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                      BUILD_BYPRODUCTS ${UDFREAD_LIBRARY}
                      BUILD_IN_SOURCE 1)

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
