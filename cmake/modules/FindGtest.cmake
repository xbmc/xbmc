#.rst:
# FindGtest
# --------
# Finds the gtest library
#
# This will define the following variables::
#
# GTEST_FOUND - system has gtest
# GTEST_INCLUDE_DIRS - the gtest include directories
# GTEST_LIBRARIES - the gtest libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GTEST gtest>=1.10.0 QUIET)
endif()

find_library(GTEST_LIBRARY NAMES gtest
                           PATHS ${PC_GTEST_LIBDIR})

find_path(GTEST_INCLUDE_DIR NAMES gtest/gtest.h
                            PATHS ${PC_GTEST_INCLUDEDIR})

set(GTEST_VERSION ${PC_GTEST_VERSION})

if (NOT GTEST_LIBRARY AND NOT GTEST_INCLUDE_DIR AND NOT GTEST_VERSION)
  set(ENABLE_INTERNAL_GTEST ON)
  message(STATUS "libgtest not found, falling back to internal build")
endif()

if(ENABLE_INTERNAL_GTEST)
  include(ExternalProject)

  set(GTEST_VERSION 1.10.0)
  set(GTEST_HASH SHA256=9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb)

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(GTEST_URL)
    get_filename_component(GTEST_URL "${GTEST_URL}" ABSOLUTE)
  else()
    set(GTEST_URL http://mirrors.kodi.tv/build-deps/sources/googletest-${GTEST_VERSION}.tar.gz)
  endif()

  if(VERBOSE)
    message(STATUS "GTEST_URL: ${GTEST_URL}")
  endif()

  set(GTEST_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libgtest.a)
  set(GTEST_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)

  externalproject_add(gtest
                      URL ${GTEST_URL}
                      URL_HASH ${GTEST_HASH}
                      DOWNLOAD_NAME gtest-${GTEST_VERSION}.tar.gz
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/gtest
                      INSTALL_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                      CMAKE_ARGS -DBUILD_GMOCK=OFF -DINSTALL_GTEST=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=lib
                      BUILD_BYPRODUCTS ${GTEST_LIBRARY})

  set_target_properties(gtest PROPERTIES FOLDER "External Projects")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gtest
                                  REQUIRED_VARS GTEST_LIBRARY GTEST_INCLUDE_DIR
                                  VERSION_VAR GTEST_VERSION)

mark_as_advanced(GTEST_INCLUDE_DIR GTEST_LIBRARY)
