#.rst:
# FindZSTD
# ----------
# Finds the ZSTD library
#
# This will define the following target:
#
#   LIBRARY::ZSTD   - The App specific library dependency target
#

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildmacroZSTD)

    if(CORE_SYSTEM_NAME STREQUAL android)
      set(EXTRA_ARGS -DANDROID_PLATFORM_LEVEL=${TARGET_MINSDK})
    endif()

    set(CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                   -DZSTD_BUILD_STATIC=ON
                   -DZSTD_BUILD_SHARED=OFF
                   -DZSTD_LEGACY_SUPPORT=OFF
                   -DZSTD_BUILD_PROGRAMS=OFF
                   -DZSTD_BUILD_TESTS=OFF
                   ${EXTRA_ARGS})

    # This can be removed when a release newer than 1.5.7 is bumped
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SOURCE_SUBDIR build/cmake)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC zstd)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_ZSTD) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET zstd::libzstd AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS zstd::libzstd)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE LIBRARY)
      SETUP_BUILD_TARGET()

      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # Set LIB_BUILD property to allow calling modules to know we will be building
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)

      # Allow calling modules to use the include dir and library when building
      # their own targets
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
        ZSTD_INCLUDE_DIR "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}"
        ZSTD_LIBRARY "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY}"
      )
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(ZSTD_FIND_REQUIRED)
      message(FATAL_ERROR "ZSTD libraries were not found. Try -DENABLE_INTERNAL_ZSTD=ON")
    endif()
  endif()
endif()
