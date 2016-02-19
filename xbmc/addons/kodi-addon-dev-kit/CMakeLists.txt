project(kodi-addon-dev-kit)

cmake_minimum_required(VERSION 3.0)

find_package(kodi REQUIRED)

add_subdirectory(kodi-addon-executable)
add_subdirectory(kodi-addon-sharedlibrary)
