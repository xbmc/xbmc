#.rst:
# FindZSTD
# ----------
# Finds the ZSTD library
#
# This will define the following target:
#
#   LIBRARY::ZSTD   - The App specific library dependency target
#

if(NOT TARGET LIBRARY::ZSTD)

  macro(buildmacroZSTD)

    set(PATCH_COMMAND ${CMAKE_COMMAND} -E copy
                      ${CORE_SOURCE_DIR}/tools/depends/target/zstd/CMakeLists.txt
                      <SOURCE_DIR>)

    set(CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                   -DZSTD_BUILD_STATIC=ON
                   -DZSTD_BUILD_SHARED=OFF
                   -DZSTD_LEGACY_SUPPORT=OFF
                   -DZSTD_BUILD_PROGRAMS=OFF
                   -DZSTD_BUILD_TESTS=OFF)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC zstd)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  # Search for cmake config. Suitable for all platforms including windows
  find_package(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                                                         HINTS ${DEPENDS_PATH}/lib/cmake
                                                         ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

  # cmake config may not be available. fallback to pkgconfig for non windows platforms
  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} libzstd${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  # Check for existing Zstd. If version < ZSTD-VERSION file version, build it
  if("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ZSTD_FIND_REQUIRED)
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if((TARGET zstd::libzstd_static OR TARGET zstd::libzstd_shared) AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # Preference for static lib if available
      if(TARGET zstd::libzstd_static)
        set(_target zstd::libzstd_static)
      else()
        set(_target zstd::libzstd_shared)
      endif()

      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_target})
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE LIBRARY)
      SETUP_BUILD_TARGET()

      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # Set LIB_BUILD property to allow calling modules to know we will be building
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(ZSTD_FIND_REQUIRED)
      message(FATAL_ERROR "ZSTD libraries were not found.")
    endif()
  endif()
endif()
