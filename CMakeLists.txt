project(updater)

cmake_minimum_required(VERSION 2.6)

include_directories(external)
include_directories(external/TinyThread/source)

add_subdirectory(src)
add_subdirectory(external/AnyOption)
add_subdirectory(external/minizip)
add_subdirectory(external/tinyxml)
add_subdirectory(external/TinyThread)

