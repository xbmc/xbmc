cmake_minimum_required(VERSION 3.13)
project(kodi-target-depends LANGUAGES CXX C)

if(NOT CORE_SYSTEM_NAME)
	string(TOLOWER ${CMAKE_SYSTEM_NAME} CORE_SYSTEM_NAME)
endif()

get_filename_component(CMAKE_SYSROOT ${CMAKE_SYSROOT} ABSOLUTE)
configure_file(Toolchain.cmake.in ${CMAKE_INSTALL_PREFIX}/Toolchain.cmake @ONLY)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_INSTALL_PREFIX}/Toolchain.cmake)

file(STRINGS ${PROJECT_SOURCE_DIR}/../../../../tools/depends/target/ffmpeg/FFMPEG-VERSION FFMPEG)
string(REGEX MATCH "VERSION=[^ ]*$.*" FFMPEG_VERSION "${FFMPEG}")
list(GET FFMPEG_VERSION 0 FFMPEG_VERSION)
string(SUBSTRING "${FFMPEG_VERSION}" 8 -1 FFMPEG_VERSION)
string(REGEX MATCH "BASE_URL=([^ ]*)" FFMPEG_BASE_URL "${FFMPEG}")
list(GET FFMPEG_BASE_URL 0 FFMPEG_BASE_URL)
string(SUBSTRING "${FFMPEG_BASE_URL}" 9 -1 FFMPEG_BASE_URL)
set(FFMPEG_URL ${FFMPEG_BASE_URL}/archive/${FFMPEG_VERSION}.tar.gz)
configure_file(common/ffmpeg/ffmpeg.txt.in ${PROJECT_SOURCE_DIR}/common/ffmpeg/ffmpeg.txt)

include(ExternalProject)

file(TO_CMAKE_PATH ${BUILD_DIR} BUILD_DIR)
set(CORE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../../..)
include(${CORE_SOURCE_DIR}/cmake/scripts/common/HandleDepends.cmake)
add_addon_depends(kodi-target-depends ${PROJECT_SOURCE_DIR})
