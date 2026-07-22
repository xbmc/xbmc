# FindCea
# --------
# Finds the internal CEA library for Kodi
#
# This will define the following imported target:
#
#   ${APP_NAME_LC}::Cea   - The CEA library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  # Build macro for CEA
  macro(buildmacroCea)


    # Always build static internal library
    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   ${EXTRA_ARGS})

    # Enable ccache if configured
    if(NOT CMAKE_CXX_COMPILER_LAUNCHER STREQUAL "")
      list(APPEND CMAKE_ARGS -DBUILD_WITH_CCACHE=ON)
    endif()

    # Build the dependency using Kodi’s build helper
    BUILD_DEP_TARGET()
  endmacro()

  # Include Kodi module helpers
  include(cmake/scripts/common/ModuleHelpers.cmake)

  # Lowercase module name for internal tracking
  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC cea)

  # Setup build and search variables
  SETUP_BUILD_VARS()
  SETUP_FIND_SPECS()
  SEARCH_EXISTING_PACKAGES()

  # Check if an existing CEA package satisfies the version
  # If not, or if internal build is enabled, build CEA
  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_CEA) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_CEA) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  # After build or find, create an imported alias target for Kodi
  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    # Alias the target to the static library built by CMake
    if(TARGET cea AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS cea)
    else()
      # Ensure proper dependency if target was built internally
      SETUP_BUILD_TARGET()
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    # Handle multi-config generators
    ADD_MULTICONFIG_BUILDMACRO()
  else()
    # If the library is required and not found, fail
    if(Cea_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find or build CEA library. Try -DENABLE_INTERNAL_CEA=ON")
    endif()
  endif()

endif()
