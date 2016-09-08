cmake_minimum_required(VERSION 3.1)
project(mingw)

function(generate_mingw32_wrapper cmd)
  set(CMD ${cmd})
  configure_file(${PROJECT_SOURCE_DIR}/mingw32-cmd.bat.in ${MINGW_PATH}/bin/${CMD}.bat @ONLY)
endfunction()

get_filename_component(CORE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../../../../.. REALPATH)

set(MSYS_PATH "${CORE_SOURCE_DIR}/project/BuildDependencies/msys64")
set(MINGW_PATH "${MSYS_PATH}/mingw32")

# configure the MinGW toolchain file
configure_file(${PROJECT_SOURCE_DIR}/Toolchain_mingw32.cmake.in ${CMAKE_INSTALL_PREFIX}/Toolchain_mingw32.cmake @ONLY)

# configure MinGWConfig.cmake
configure_file(${PROJECT_SOURCE_DIR}/MinGWConfig.cmake.in ${CMAKE_INSTALL_PREFIX}/MinGWConfig.cmake)

# TODO: MinGW GCC 5.3.0-1 comes without cc.exe, Remove this once package is bumped to 5.3.0-p2
# See https://github.com/Alexpux/MINGW-packages/pull/1034
if(NOT EXISTS ${MINGW_PATH}/bin/cc.exe)
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${MINGW_PATH}/bin/gcc.exe ${MINGW_PATH}/bin/cc.exe)
endif()

# configure the MinGW wrapper batch scripts
generate_mingw32_wrapper("make")
generate_mingw32_wrapper("gcc")
generate_mingw32_wrapper("cc")
generate_mingw32_wrapper("g++")
generate_mingw32_wrapper("ar")
generate_mingw32_wrapper("ld")
generate_mingw32_wrapper("windres")
