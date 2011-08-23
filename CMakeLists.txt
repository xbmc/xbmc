project(updater)

cmake_minimum_required(VERSION 2.6)

include_directories(external)
include_directories(external/TinyThread/source)

if (WIN32)
include_directories(external/zlib/)
endif()

if (APPLE)
	set(CMAKE_OSX_ARCHITECTURES i386;x86_64)
endif()

add_subdirectory(src)
add_subdirectory(external/AnyOption)
add_subdirectory(external/minizip)
add_subdirectory(external/tinyxml)
add_subdirectory(external/TinyThread)

