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
      # pkg_check_modules(... IMPORTED_TARGET) can leave a bare "-lgtest" on the
      # target's link interface if it fails to resolve the library to a full path
      # itself. Resolve it explicitly and patch the interface so linking never
      # depends on the linker's implicit default search path.
      find_library(GTEST_LIBRARY NAMES gtest
                                 HINTS ${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LIBRARY_DIRS})
      mark_as_advanced(GTEST_LIBRARY)

      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})

      if(GTEST_LIBRARY)
        get_target_property(_gtest_link_libraries PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_LINK_LIBRARIES)
        if(_gtest_link_libraries)
          list(TRANSFORM _gtest_link_libraries REPLACE "^-lgtest$" "${GTEST_LIBRARY}")
          set_target_properties(PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} PROPERTIES
                                                                                     INTERFACE_LINK_LIBRARIES "${_gtest_link_libraries}")
        endif()
        unset(_gtest_link_libraries)
      endif()
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
