#.rst:
# FindGtest
# --------
# Finds the gtest library
#
# This will define the following imported targets::
#
#   LIBRARY::Gtest   - The gtest library

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroGtest)

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)

    if(WIN32 OR WINDOWS_STORE)
      set(CMAKE_OPTIONS -Dgtest_force_shared_crt=ON)

      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-win-stl_algorithms_cxxflag.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/02-win-pdb_install_paths.patch")
      generate_patchcommand("${patches}")
      unset(patches)
    endif()

    if(CORE_SYSTEM_NAME STREQUAL "osx")
      # Generator explression for semi colon required to pass the quoted list through.
      set(CMAKE_OPTIONS -DCMAKE_OSX_ARCHITECTURES=arm64$<SEMICOLON>x86_64)
    endif()

    set(CMAKE_ARGS -DBUILD_GMOCK=OFF
                   -DINSTALL_GTEST=ON
                   -DBUILD_SHARED_LIBS=OFF
                   -DCMAKE_DEBUG_POSTFIX=${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX}
                   ${CMAKE_OPTIONS})

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC gtest)
  # search calls are case sensitive
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME GTest)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME_PC gtest)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_GTEST)
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    # cmake config found
    if(TARGET GTest::gtest AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS GTest::gtest)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE LIBRARY)
      SETUP_BUILD_TARGET()

      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Gtest_FIND_REQUIRED)
      message(FATAL_ERROR "Gtest libraries were not found. You can either disable testing with -DENABLE_TESTING=OFF or build gtest with -DENABLE_INTERNAL_GTEST=ON")
    endif()
  endif()
endif()
