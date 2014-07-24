project(xbmc-addons)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(ExternalProject)

### setup all the necessary paths
if(NOT XBMCROOT)
  set(XBMCROOT ${PROJECT_SOURCE_DIR}/../../..)
else()
  file(TO_CMAKE_PATH "${XBMCROOT}" XBMCROOT)
endif()
get_filename_component(XBMCROOT "${XBMCROOT}" ABSOLUTE)

if(NOT WIN32)
  if(NOT DEPENDS_PATH)
    set(DEPENDS_PATH "${PROJECT_SOURCE_DIR}/output/depends")
  else()
    file(TO_CMAKE_PATH "${DEPENDS_PATH}" DEPENDS_PATH)
  endif()

  # make sure CMAKE_PREFIX_PATH is set
  if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${DEPENDS_PATH}")
  else()
    file(TO_CMAKE_PATH "${CMAKE_PREFIX_PATH}" CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${DEPENDS_PATH}")
  endif()
endif()

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/output/addons")
endif()
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

set(BUILD_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
               -DCMAKE_BUILD_TYPE=Release
               -DCMAKE_USER_MAKE_RULES_OVERRIDE=${CMAKE_USER_MAKE_RULES_OVERRIDE}
               -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=${CMAKE_USER_MAKE_RULES_OVERRIDE_CXX}
               -DBUILD_SHARED_LIBS=1)

if(PACKAGE_ZIP)
  # needed for project installing
  list(APPEND BUILD_ARGS -DPACKAGE_ZIP=1)
  MESSAGE("package zip specified")
endif()

if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND BUILD_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
  MESSAGE("toolchain specified")
  MESSAGE(${BUILD_ARGS})
endif()

if(NOT ADDONS_TO_BUILD)
  set(ADDONS_TO_BUILD "all")
else()
  message(STATUS "Building following addons: ${ADDONS_TO_BUILD}")
  separate_arguments(ADDONS_TO_BUILD)
endif()

if(NOT WIN32)
  # copy the xbmc-prepare-env.cmake script to the depends path so that we can include it
  file(COPY ${XBMCROOT}/project/cmake/scripts/common/xbmc-prepare-env.cmake DESTINATION ${DEPENDS_PATH}/lib/xbmc)

  # add the location of xbmc-prepare-env.cmake to CMAKE_MODULE_PATH so that it is found
  list(APPEND CMAKE_MODULE_PATH ${DEPENDS_PATH}/lib/xbmc)

  # include xbmc-prepare-env.cmake which contains the logic to install the addon header bindings etc
  include(xbmc-prepare-env)
endif()

### get and build all the binary addons
# look for all the addons to be built
file(GLOB addons ${PROJECT_SOURCE_DIR}/addons/*.txt)
foreach(addon ${addons})
  file(STRINGS ${addon} def)
  string(REPLACE " " ";" def ${def})
  list(GET def 0 id)
  list(GET def 1 url)
  list(GET def 2 revision)

  list(FIND ADDONS_TO_BUILD ${id} idx)
  if(idx GREATER -1 OR ADDONS_TO_BUILD STREQUAL "all")
    # make sure the output directory is clean
    if(EXISTS "${CMAKE_INSTALL_PREFIX}/${id}")
      file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/${id}/")
    endif()

    # add the addon as an external project for automatic building
    externalproject_add(${id}
                      GIT_REPOSITORY ${url}
                      GIT_TAG ${revision}
                      INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
                      CMAKE_ARGS ${BUILD_ARGS}
                     )
  endif()
endforeach()
