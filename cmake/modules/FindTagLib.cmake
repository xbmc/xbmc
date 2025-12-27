#.rst:
# FindTagLib
# ----------
# Finds the TagLib library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::TagLib   - The TagLib library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroTagLib)
    # Darwin systems use a system tbd that isnt found as a static lib
    # Other platforms when using ENABLE_INTERNAL_TAGLIB, we want the static lib
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      # Requires cmake 3.24 for ZLIB_USE_STATIC_LIBS to actually do something
      set(ZLIB_USE_STATIC_LIBS ON)
    endif()
    find_package(ZLIB REQUIRED ${SEARCH_QUIET})
    find_package(Utfcpp REQUIRED ${SEARCH_QUIET})
  
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  
    if(WIN32 OR WINDOWS_STORE)
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/001-cmake-pdb-debug.patch")
      generate_patchcommand("${patches}")
      unset(patches)

      if(WINDOWS_STORE)
        set(EXTRA_ARGS -DPLATFORM_WINRT=ON)
      endif()
    endif()
  
    # Debug postfix only used for windows
    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX "d")
    endif()
  
    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DBUILD_EXAMPLES=OFF
                   -DBUILD_TESTING=OFF
                   -DBUILD_BINDINGS=OFF
                   ${EXTRA_ARGS})
  
    BUILD_DEP_TARGET()
  
    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} LIBRARY::ZLIB
                                                                        LIBRARY::Utfcpp)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES "LIBRARY::ZLIB")
  endmacro()

  # If there is a potential this library can be built internally
  # Check its dependencies to allow forcing this lib to be built if one of its
  # dependencies requires being rebuilt
  if(ENABLE_INTERNAL_TAGLIB)
    # Dependency list of this find module for an INTERNAL build
    set(${CMAKE_FIND_PACKAGE_NAME}_DEPLIST Utfcpp)

    check_dependency_build(${CMAKE_FIND_PACKAGE_NAME} "${${CMAKE_FIND_PACKAGE_NAME}_DEPLIST}")
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC taglib)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  # Taglib 2.0+ provides cmake configs
  find_package(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                                                         HINTS ${DEPENDS_PATH}/lib/cmake
                                                         ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

  # cmake config may not be available (taglib 1.x series)
  # fallback to pkgconfig for non windows platforms
  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)

    else()
      # Taglib installs a shell script for all platforms. This can provide version universally
      find_program(TAGLIB-CONFIG NAMES taglib-config taglib-config.cmd
                                 HINTS ${DEPENDS_PATH}/bin)
    
      if(TAGLIB-CONFIG)
        execute_process(COMMAND "${TAGLIB-CONFIG}" --version
                        OUTPUT_VARIABLE ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
      endif()
    endif()
  endif()

  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_TAGLIB) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_TAGLIB) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET TagLib::tag AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS TagLib::tag)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS TAGLIB_STATIC)
      ADD_TARGET_COMPILE_DEFINITION()
    endif()
  else()
    if(TagLib_FIND_REQUIRED)
      message(FATAL_ERROR "TagLib not found. You may want to try -DENABLE_INTERNAL_TAGLIB=ON")
    endif()
  endif()
endif()
