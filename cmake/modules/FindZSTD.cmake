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

    if(UNIX AND NOT CORE_SYSTEM_NAME STREQUAL android)
      find_package(Threads REQUIRED)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES Threads::Threads)
    endif()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC zstd)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME_PC libzstd)

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

  if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
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
  elseif(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    foreach(_zstd_target zstd::libzstd zstd::libzstd_shared zstd::libzstd_static
                         PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      if(TARGET ${_zstd_target})
        get_target_property(_zstd_aliased_target ${_zstd_target} ALIASED_TARGET)
        if(_zstd_aliased_target)
          set(_zstd_target ${_zstd_aliased_target})
        endif()
        add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_zstd_target})
        break()
      endif()
    endforeach()
    unset(_zstd_target)
    unset(_zstd_aliased_target)
  endif()

  if(TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})
    ADD_MULTICONFIG_BUILDMACRO()
  endif()
endif()

set(ZSTD_FOUND FALSE)
if(TARGET LIBRARY::ZSTD)
  set(ZSTD_FOUND TRUE)
elseif(ZSTD_FIND_REQUIRED)
  message(FATAL_ERROR "ZSTD libraries were not found. Try -DENABLE_INTERNAL_ZSTD=ON")
endif()
