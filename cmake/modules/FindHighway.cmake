#.rst:
# FindHighway
# ----------
# Finds the libhighway library
#
# This will define the following target ALIAS:
#
#   hwy::hwy   - The highway library
#   LIBRARY::Highway    - ALIAS target to hwy::hwy
#

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildmacroHighway)

    set(CMAKE_ARGS -DHWY_ENABLE_CONTRIB=OFF
                   -DHWY_ENABLE_EXAMPLES=OFF
                   -DHWY_ENABLE_TESTS=OFF
                   -DHWY_TEST_STANDALONE=OFF)

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR ${DEPENDS_PATH}/include/hwy)

    # We need the dir created to avoid non-existant path errors
    file(MAKE_DIRECTORY ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR})

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC highway)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME hwy)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET hwy::hwy AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS hwy::hwy)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME)
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE LIBRARY)
      SETUP_BUILD_TARGET()

      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Highway_FIND_REQUIRED)
      message(FATAL_ERROR "Highway library was not found.")
    endif()
  endif()
endif()
