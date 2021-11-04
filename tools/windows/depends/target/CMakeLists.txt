cmake_minimum_required(VERSION 3.13)
project(kodi-target-depends LANGUAGES CXX C)

if(NOT CORE_SYSTEM_NAME)
	string(TOLOWER ${CMAKE_SYSTEM_NAME} CORE_SYSTEM_NAME)
endif()

get_filename_component(CMAKE_SYSROOT ${CMAKE_SYSROOT} ABSOLUTE)
configure_file(Toolchain.cmake.in ${CMAKE_INSTALL_PREFIX}/Toolchain.cmake @ONLY)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_INSTALL_PREFIX}/Toolchain.cmake)

include(ExternalProject)

file(TO_CMAKE_PATH ${BUILD_DIR} BUILD_DIR)
set(CORE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../../..)
include(${CORE_SOURCE_DIR}/cmake/scripts/common/HandleDepends.cmake)
add_addon_depends(kodi-target-depends ${PROJECT_SOURCE_DIR})
