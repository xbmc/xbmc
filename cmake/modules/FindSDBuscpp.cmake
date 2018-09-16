#.rst:
# FindSDBUSCPP
# -------
# Finds the sdbuscpp library
#
# This will define the following variables::
#
# SDBUSCPP_FOUND        - system has SDBUSCPP
# SDBUSCPP_INCLUDE_DIRS - the SDBUSCPP include directory
# SDBUSCPP_LIBRARIES    - the SDBUSCPP libraries
# SDBUSCPP_DEFINITIONS  - the SDBUSCPP definitions
#

if(ENABLE_INTERNAL_SDBUSCPP)

  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/sdbuscpp/Makefile VER REGEX "^[ ]*VERSION[ ]*=.+$")
  string(REGEX REPLACE "^[ ]*VERSION[ ]*=[ ]*" "" SDBUSCPP_VERSION "${VER}")

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(SDBUSCPP_URL)
      get_filename_component(SDBUSCPP_URL "${SDBUSCPP_URL}" ABSOLUTE)
  else()
      set(SDBUSCPP_URL http://mirrors.kodi.tv/build-deps/sources/sdbus-cpp-${SDBUSCPP_VERSION}.tar.gz)
  endif()

  if(VERBOSE)
      message(STATUS "SDBUSCPP_URL: ${SDBUSCPP_URL}")
  endif()

  set(SDBUSCPP_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib*/libsdbus-c++.a)
  set(SDBUSCPP_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)

  include(ExternalProject)
  externalproject_add(sdbuscpp
                      URL ${SDBUSCPP_URL}
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/sdbuscpp
                      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                 -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                                 -DCMAKE_INSTALL_LIBDIR=lib
                                 -DBUILD_LIBSYSTEMD=OFF
                                 -DBUILD_SHARED_LIBS=OFF
                                 -DBUILD_TESTS=OFF
                                 -DBUILD_CODE_GEN=OFF
                                 -DBUILD_DOC=OFF
                      BUILD_BYPRODUCTS ${SDBUSCPP_LIBRARY})

  set_target_properties(sdbuscpp PROPERTIES FOLDER "External Projects")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SDBuscpp
                                    REQUIRED_VARS SDBUSCPP_LIBRARY SDBUSCPP_INCLUDE_DIR
                                    VERSION_VAR SDBUSCPP_VERSION)

  if(SDBUSCPP_FOUND)
    set(SDBUSCPP_LIBRARIES ${SDBUSCPP_LIBRARY} systemd)
    set(SDBUSCPP_INCLUDE_DIRS ${SDBUSCPP_INCLUDE_DIR} ${SDBUSCPP_ARCH_INCLUDE_DIR})
    set(SDBUSCPP_DEFINITIONS -DHAS_SDBUSCPP=1)
  endif()

else()

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SDBUSCPP sdbus-c++ QUIET)
endif()

find_path(SDBUSCPP_INCLUDE_DIR NAMES sdbus-c++.h
                           PATH_SUFFIXES sdbus-c++
                           PATHS ${PC_SDBUSCPP_INCLUDE_DIR})
find_library(SDBUSCPP_LIBRARY NAMES sdbus-c++
                          PATHS ${PC_SDBUSCPP_LIBDIR})

set(SDBUSCPP_VERSION ${PC_SDBUSCPP_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDBuscpp
                                  REQUIRED_VARS SDBUSCPP_LIBRARY SDBUSCPP_INCLUDE_DIR
                                  VERSION_VAR SDBUSCPP_VERSION)

if(SDBUSCPP_FOUND)
  set(SDBUSCPP_LIBRARIES ${SDBUSCPP_LIBRARY})
  set(SDBUSCPP_INCLUDE_DIRS ${SDBUSCPP_INCLUDE_DIR} ${SDBUSCPP_ARCH_INCLUDE_DIR})
  set(SDBUSCPP_DEFINITIONS -DHAS_SDBUSCPP=1)
endif()

endif()

mark_as_advanced(SDBUSCPP_INCLUDE_DIR SDBUSCPP_LIBRARY)
