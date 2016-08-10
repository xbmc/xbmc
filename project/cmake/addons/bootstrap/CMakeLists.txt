cmake_minimum_required(VERSION 3.1)
project(kodi-addons-bootstrap)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

# make sure CMAKE_INSTALL_PREFIX is properly set
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/../addons")
endif()
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

# figure out where the build directory is located
if(NOT BUILD_DIR)
  set(BUILD_DIR "${CMAKE_BINARY_DIR}/build")
else()
  file(TO_CMAKE_PATH "${BUILD_DIR}" BUILD_DIR)
endif()
get_filename_component(BUILD_DIR "${BUILD_DIR}" ABSOLUTE)

# make sure that the repositories to build have been specified
if(NOT REPOSITORY_TO_BUILD)
  set(REPOSITORY_TO_BUILD_DEFAULT ON)
  set(REPOSITORY_TO_BUILD "all")
  set(REPOSITORY_REVISION "")
  message(STATUS "Bootstrapping all repositories")
else()
  set(REPOSITORY_TO_BUILD_DEFAULT OFF)
  message(STATUS "Bootstrapping following repository: ${REPOSITORY_TO_BUILD}")
endif()

# figure out which addons to bootstrap (defaults to all)
if(NOT ADDONS_TO_BUILD)
  set(ADDONS_TO_BUILD "all")
  message(STATUS "Bootstrapping all addons")
else()
  message(STATUS "Bootstrapping following addons: ${ADDONS_TO_BUILD}")
endif()

include(ExternalProject)

function(bootstrap_repo repo_id repo_url repo_revision)
  message(STATUS "Bootstrapping addons from ${repo_id} (${repo_url} ${repo_revision})...")
  externalproject_add(${repo_id}
                      GIT_REPOSITORY ${repo_url}
                      GIT_TAG ${repo_revision}
                      PREFIX ${BUILD_DIR}/${repo_id}
                      CONFIGURE_COMMAND ""
                      BUILD_COMMAND ""
                      INSTALL_COMMAND ${CMAKE_COMMAND}
                                      -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
                                      -DPROJECT_SOURCE_DIR=<SOURCE_DIR>
                                      -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                                      -DADDONS_TO_BUILD=${ADDONS_TO_BUILD}
                                      -P ${PROJECT_SOURCE_DIR}/Bootstrap.cmake
                      )
endfunction()

# look for all addons repository definitions
set(REPOSITORY_TO_BUILD_FOUND OFF)
file(GLOB repos repositories/*.txt)
foreach(repo ${repos})
  file(STRINGS ${repo} repo_definition)
  separate_arguments(repo_definition)
  list(GET repo_definition 0 repo_id)

  list(FIND REPOSITORY_TO_BUILD ${repo_id} idx)
  if(idx GREATER -1 OR REPOSITORY_TO_BUILD STREQUAL "all")
    set(REPOSITORY_TO_BUILD_FOUND ON)

    # get the URL of the repository
    list(GET repo_definition 1 repo_url)

    # get the revision of the repository if not provided as an argument
    if(NOT REPOSITORY_REVISION)
      list(GET repo_definition 2 repo_revision)
    else()
      set(repo_revision "${REPOSITORY_REVISION}")
    endif()

    bootstrap_repo(${repo_id} ${repo_url} ${repo_revision})
  endif()
endforeach()

# if we have been asked to bootstrap a specific repository (not the default one) and
# it couldn't be found in the predefined repository definitions we assume that it's a
# URL to a specific repository
if(NOT REPOSITORY_TO_BUILD_DEFAULT AND NOT REPOSITORY_TO_BUILD_FOUND)
  # default to the master branch if no revision has been provided
  if(NOT REPOSITORY_REVISION)
    set(REPOSITORY_REVISION "master")
  endif()

  bootstrap_repo(binary-addons-custom ${REPOSITORY_TO_BUILD} ${REPOSITORY_REVISION})
endif()
