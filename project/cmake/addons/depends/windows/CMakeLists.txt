project(kodi-addons-depends-windows)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(ExternalProject)

if(NOT DEPENDS_PATH)
  message(FATAL_ERROR "DEPENDS_PATH (${DEPENDS_PATH}) is not a valid target directory.")
else()
  file(TO_CMAKE_PATH "${DEPENDS_PATH}" DEPENDS_PATH)
endif()
get_filename_component(DEPENDS_PATH "${DEPENDS_PATH}" ABSOLUTE)
list(APPEND CMAKE_PREFIX_PATH ${DEPENDS_PATH})

if(NOT DEPENDS_TO_BUILD)
  set(DEPENDS_TO_BUILD "all")
endif()

function(add_internal id url inputfile)
  externalproject_add(${id}
                      URL ${url}
                      PREFIX build/${id}
                      CONFIGURE_COMMAND ""
                      BUILD_COMMAND ""
                      INSTALL_COMMAND ${CMAKE_COMMAND}
                                      -DINPUTDIR=${PROJECT_BINARY_DIR}/build/${id}/src/${id}
                                      -DINPUTFILE=${inputfile}
                                      -DDESTDIR=${DEPENDS_PATH}
                                      -P ${PROJECT_SOURCE_DIR}/install.cmake
                     )
endfunction()

#find_package(7Zip REQUIRED)

file(GLOB_RECURSE download_input_files prebuilt/*.txt)
foreach(file ${download_input_files})
  if(NOT file MATCHES install.txt)
    file(STRINGS ${file} def)
    get_filename_component(dir ${file} PATH)
    separate_arguments(def)
    list(GET def 0 id)

    list(FIND DEPENDS_TO_BUILD ${id} idx)
    if(idx GREATER -1 OR DEPENDS_TO_BUILD STREQUAL "all")
      list(GET def 1 url)
      add_internal(${id} ${url} ${dir}/install.txt)
    endif()
  endif()
endforeach()
