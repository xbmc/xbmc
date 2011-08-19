project(updater)

cmake_minimum_required(VERSION 2.6)

include_directories(external)

add_subdirectory(src)
add_subdirectory(external/AnyOption)
add_subdirectory(external/minizip)
add_subdirectory(external/tinyxml)

