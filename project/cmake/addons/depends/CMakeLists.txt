project(kodi-addons-depends)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT CORE_SYSTEM_NAME)
  string(TOLOWER ${CMAKE_SYSTEM_NAME} CORE_SYSTEM_NAME)
endif()

include(ExternalProject)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/../output/depends)
else()
  file(TO_CMAKE_PATH "${CMAKE_INSTALL_PREFIX}" CMAKE_INSTALL_PREFIX)
endif()
get_filename_component(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

if(NOT DEPENDS_TO_BUILD)
  set(DEPENDS_TO_BUILD "all")
else()
  message(STATUS "Building following dependencies: ${DEPENDS_TO_BUILD}")
  separate_arguments(DEPENDS_TO_BUILD)
endif()

## handle dependencies with a cmake based buildsystem which need to be downloaded, built and installed
file(GLOB_RECURSE cmake_input_files ${CORE_SYSTEM_NAME}/cmake/*.txt)
file(GLOB_RECURSE cmake_input_files2 common/*.txt)
list(APPEND cmake_input_files ${cmake_input_files2})
foreach(file ${cmake_input_files})
  if(NOT (file MATCHES CMakeLists.txt OR
          file MATCHES install.txt OR
          file MATCHES noinstall.txt OR
          file MATCHES flags.txt OR
          file MATCHES deps.txt))
    message(STATUS "Processing ${file}")
    file(STRINGS ${file} def)
    separate_arguments(def)
    list(LENGTH def deflength)
    get_filename_component(dir ${file} PATH)

    # get the id and url of the dependency
    set(url "")
    if(NOT "${def}" STREQUAL "")
      # read the id and the url from the file
      list(GET def 0 id)
      if(deflength GREATER 1)
        list(GET def 1 url)
        message(STATUS "${id} url: ${url}")
      endif()
    else()
      # read the id from the filename
      get_filename_component(id ${file} NAME_WE)
    endif()

    list(FIND DEPENDS_TO_BUILD ${id} idx)
    if(idx GREATER -1 OR DEPENDS_TO_BUILD STREQUAL "all")
      # check if there are any library specific flags that need to be passed on
      if(EXISTS ${dir}/flags.txt})
        file(STRINGS ${dir}/flags.txt extraflags)
        message(STATUS "${id} extraflags: ${extraflags}")
      endif()

      set(BUILD_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
                     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                     -DCMAKE_USER_MAKE_RULES_OVERRIDE=${CMAKE_USER_MAKE_RULES_OVERRIDE}
                     -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=${CMAKE_USER_MAKE_RULES_OVERRIDE_CXX}
                     -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                     -DARCH_DEFINES=${ARCH_DEFINES}
                     -DENABLE_STATIC=1
                     -DBUILD_SHARED_LIBS=0
                     "${extraflags}")

      if(CMAKE_TOOLCHAIN_FILE)
        list(APPEND BUILD_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        MESSAGE("toolchain specified")
        MESSAGE(${BUILD_ARGS})
      endif()

      # if there's a CMakeLists.txt use it to prepare the build
      if(EXISTS ${dir}/CMakeLists.txt)
        set(PATCH_COMMAND ${CMAKE_COMMAND} -E copy
                          ${dir}/CMakeLists.txt
                          ${CMAKE_BINARY_DIR}/build/${id}/src/${id})
      else()
        set(PATCH_COMMAND "")
      endif()

      # if there's an install.txt use it to properly install the built files
      if(EXISTS ${dir}/install.txt)
        set(INSTALL_COMMAND INSTALL_COMMAND ${CMAKE_COMMAND}
                                            -DINPUTDIR=${CMAKE_BINARY_DIR}/build/${id}/src/${id}-build/
                                            -DINPUTFILE=${dir}/install.txt
                                            -DDESTDIR=${CMAKE_INSTALL_PREFIX}
                                            -DENABLE_STATIC=1
                                            "${extraflags}"
                                            -P ${PROJECT_SOURCE_DIR}/install.cmake)
      elseif(EXISTS ${dir}/noinstall.txt)
        set(INSTALL_COMMAND INSTALL_COMMAND "")
      else()
        set(INSTALL_COMMAND "")
      endif()

      # prepare the setup of the call to externalproject_add()
      set(EXTERNALPROJECT_SETUP PREFIX build/${id}
                                CMAKE_ARGS ${BUILD_ARGS}
                                PATCH_COMMAND ${PATCH_COMMAND}
                                "${INSTALL_COMMAND}")

      # if there's an url defined we need to pass that to externalproject_add()
      if(DEFINED url AND NOT "${url}" STREQUAL "")
        # check if there's a third parameter in the file
        if(deflength GREATER 2)
          # the third parameter is considered as a revision of a git repository
          list(GET def 2 revision)

          externalproject_add(${id}
                              GIT_REPOSITORY ${url}
                              GIT_TAG ${revision}
                              "${EXTERNALPROJECT_SETUP}"
                             )
        else()
          externalproject_add(${id}
                              URL ${url}
                              "${EXTERNALPROJECT_SETUP}"
                             )
        endif()
      else()
        externalproject_add(${id}
                            SOURCE_DIR ${dir}
                            "${EXTERNALPROJECT_SETUP}"
                           )
      endif()

      # check if there's a deps.txt containing dependencies on other libraries
      if(EXISTS ${dir}/deps.txt)
        file(STRINGS ${dir}/deps.txt deps)
        message(STATUS "${id} dependencies: ${deps}")
        add_dependencies(${id} ${deps})
      endif()
    endif()
  endif()
endforeach()

## if there's a platform-specific sub-directory containing a CMakeLists.txt, add it to the build as well
if(EXISTS ${PROJECT_SOURCE_DIR}/${CORE_SYSTEM_NAME}/CMakeLists.txt)
  message(STATUS "Processing ${CORE_SYSTEM_NAME}")
  add_subdirectory(${CORE_SYSTEM_NAME})
else()
  message(FATAL_ERROR "CORE_SYSTEM_NAME: ${PROJECT_SOURCE_DIR}/${CORE_SYSTEM_NAME}/CMakeLists.txt")
endif()