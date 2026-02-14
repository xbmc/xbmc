cmake_minimum_required(VERSION 3.18)
project(zstd_shim VERSION 1.5.7)

if(ANDROID AND NOT DEFINED ANDROID_PLATFORM_LEVEL)
  set(ANDROID_PLATFORM_LEVEL 23)
endif()

add_subdirectory(build/cmake)

if(TARGET libzstd_static)
  set_target_properties(libzstd_static PROPERTIES OUTPUT_NAME zstd)
  if(NOT TARGET zstd::libzstd_static)
    add_library(zstd::libzstd_static ALIAS libzstd_static)
  endif()
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/zstd-config-version.cmake
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zstd-config-version.cmake
        DESTINATION lib/cmake/zstd)
