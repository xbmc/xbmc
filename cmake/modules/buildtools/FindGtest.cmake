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
#
# and the following imported targets:
#
#   Gtest::Gtest   - The gtest library

if(ENABLE_INTERNAL_GTEST)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC gtest)

  SETUP_BUILD_VARS()

  set(GTEST_VERSION ${${MODULE}_VER})

  # Override build type detection and always build as release
  set(GTEST_BUILD_TYPE Release)

  set(CMAKE_ARGS -DBUILD_GMOCK=OFF
                 -DINSTALL_GTEST=ON
                 -DBUILD_SHARED_LIBS=OFF
                 -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)

  BUILD_DEP_TARGET()
else()

  if(Gtest_FIND_VERSION)
    if(Gtest_FIND_VERSION_EXACT)
      set(Gtest_FIND_SPEC "=${Gtest_FIND_VERSION_COMPLETE}")
    else()
      set(Gtest_FIND_SPEC ">=${Gtest_FIND_VERSION_COMPLETE}")
    endif()
  endif()

  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GTEST gtest${Gtest_FIND_SPEC} QUIET)
    set(GTEST_VERSION ${PC_GTEST_VERSION})
  elseif(WIN32)
    set(GTEST_VERSION ${Gtest_FIND_VERSION_COMPLETE})
  endif()

  find_path(GTEST_INCLUDE_DIR NAMES gtest/gtest.h
                              HINTS ${PC_GTEST_INCLUDEDIR})

  find_library(GTEST_LIBRARY_RELEASE NAMES gtest
                                     HINTS ${PC_GTEST_LIBDIR})
  find_library(GTEST_LIBRARY_DEBUG NAMES gtestd
                                   HINTS ${PC_GTEST_LIBDIR})

  include(SelectLibraryConfigurations)
  select_library_configurations(GTEST)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gtest
                                  REQUIRED_VARS GTEST_LIBRARY GTEST_INCLUDE_DIR
                                  VERSION_VAR GTEST_VERSION)

if(GTEST_FOUND)
  set(GTEST_LIBRARIES ${GTEST_LIBRARY})
  set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIR})
endif()

if(NOT TARGET Gtest::Gtest)
  add_library(Gtest::Gtest UNKNOWN IMPORTED)
  set_target_properties(Gtest::Gtest PROPERTIES
                                     IMPORTED_LOCATION "${GTEST_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}")
endif()

mark_as_advanced(GTEST_INCLUDE_DIR GTEST_LIBRARY)
