cmake_minimum_required(VERSION 3.12)
project(kodi-addons-depends)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT CORE_SYSTEM_NAME)
  string(TOLOWER ${CMAKE_SYSTEM_NAME} CORE_SYSTEM_NAME)
endif()

include(ExternalProject)

if(NOT ADDON_DEPENDS_PATH)
  set(ADDON_DEPENDS_PATH ${PROJECT_SOURCE_DIR}/../build/depends)
else()
  file(TO_CMAKE_PATH "${ADDON_DEPENDS_PATH}" ADDON_DEPENDS_PATH)
endif()
get_filename_component(ADDON_DEPENDS_PATH "${ADDON_DEPENDS_PATH}" ABSOLUTE)
list(APPEND CMAKE_PREFIX_PATH ${ADDON_DEPENDS_PATH})

if(NOT BUILD_DIR)
    set(BUILD_DIR "${CMAKE_BINARY_DIR}/build")
else()
  file(TO_CMAKE_PATH "${BUILD_DIR}" BUILD_DIR)
endif()
get_filename_component(BUILD_DIR "${BUILD_DIR}" ABSOLUTE)

## use add_addon_depends to handle the cmake based dependencies
include(${CORE_SOURCE_DIR}/cmake/scripts/common/HandleDepends.cmake)
add_addon_depends(depends "${PROJECT_SOURCE_DIR}")

## if there's a platform-specific sub-directory containing a CMakeLists.txt, add it to the build as well
if(EXISTS ${PROJECT_SOURCE_DIR}/${CORE_SYSTEM_NAME}/CMakeLists.txt)
  message(STATUS "Processing ${CORE_SYSTEM_NAME}")
  add_subdirectory(${CORE_SYSTEM_NAME})
else()
  message(STATUS "No platform specific file ${PROJECT_SOURCE_DIR}/${CORE_SYSTEM_NAME}/CMakeLists.txt found")
endif()
