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

if(ENABLE_INTERNAL_GTEST)

  include(${WITH_KODI_DEPENDS}/packages/googletest/package.cmake)
  set(GTEST_VERSION "${PKG_VERSION}")
  add_depends_for_targets("HOST")

  add_custom_target(googletest ALL DEPENDS googletest-host)

  set(GTEST_LIBRARY ${INSTALL_PREFIX_HOST}/lib/libgtest.a)
  set(GTEST_INCLUDE_DIR ${INSTALL_PREFIX_HOST}include)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Gtest
                                    REQUIRED_VARS GTEST_LIBRARY GTEST_INCLUDE_DIR
                                    VERSION_VAR GTEST_VERSION)

  set(GTEST_LIBRARIES ${GTEST_LIBRARY})
  set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIR})
else()
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GTEST gtest>=1.10.0 QUIET)
  endif()

  find_library(GTEST_LIBRARY NAMES gtest
                             PATHS ${PC_GTEST_LIBDIR})

  find_path(GTEST_INCLUDE_DIR NAMES gtest/gtest.h
                              PATHS ${PC_GTEST_INCLUDEDIR})

  set(GTEST_VERSION ${PC_GTEST_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Gtest
                                    REQUIRED_VARS GTEST_LIBRARY GTEST_INCLUDE_DIR
                                    VERSION_VAR GTEST_VERSION)

  if(GTEST_FOUND)
    set(GTEST_LIBRARIES ${GTEST_LIBRARY})
    set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIR})
  endif()
  mark_as_advanced(GTEST_INCLUDE_DIR GTEST_LIBRARY)
endif()
