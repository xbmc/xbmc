project(xbmc-addons-depends-win32)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(ExternalProject)

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/depends)
endif()
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

function(add_internal id url inputfile)
  externalproject_add(${id}
                      URL ${url}
                      PREFIX build/${id}
                      CONFIGURE_COMMAND ""
                      BUILD_COMMAND ""
                      INSTALL_COMMAND ${CMAKE_COMMAND}
                                      -DINPUTDIR=${CMAKE_BINARY_DIR}/build/${id}/src/${id}
                                      -DINPUTFILE=${inputfile}
                                      -DDESTDIR=${CMAKE_INSTALL_PREFIX}
                                      -P ${PROJECT_SOURCE_DIR}/install.cmake)
endfunction()

function(add_7zip id url inputfile)
  externalproject_add(${id}
                      PREFIX build/${id}
                      DOWNLOAD_COMMAND ${CMAKE_COMMAND}
                                       -D7ZIP_EXECUTABLE=${7ZIP_EXECUTABLE}
                                       -DURL=${url}
                                       -DDEST=${CMAKE_BINARY_DIR}/build/${id}/src/${id}
                                       -DDESTDIR=${CMAKE_BINARY_DIR}/build/${id}/src/${id}-build
                                       -P ${PROJECT_SOURCE_DIR}/extract-7z.cmake
                      CONFIGURE_COMMAND ""
                      BUILD_COMMAND ""
                      INSTALL_COMMAND ${CMAKE_COMMAND}
                                      -DINPUTDIR=${CMAKE_BINARY_DIR}/build/${id}/src/${id}-build/
                                      -DINPUTFILE=${inputfile}
                                      -DDESTDIR=${CMAKE_INSTALL_PREFIX}
                                      -P ${PROJECT_SOURCE_DIR}/install.cmake)
endfunction()

#find_package(7Zip REQUIRED)

file(GLOB_RECURSE download_input_files prebuilt/*.txt)
foreach(file ${download_input_files})
  if(NOT file MATCHES install.txt)
    file(STRINGS ${file} def)
    get_filename_component(dir ${file} PATH)
    string(REPLACE " " ";" def ${def})
    list(GET def 0 id)
    list(GET def 1 url)
    #    if(url MATCHES .7z OR url MATCHES .lzma)
    #      add_7zip(${id} ${url} ${dir}/install.txt)
    #    else()
      add_internal(${id} ${url} ${dir}/install.txt)
      #    endif()
  endif()
endforeach()
