project(kodi-addons)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT CORE_SYSTEM_NAME)
  string(TOLOWER ${CMAKE_SYSTEM_NAME} CORE_SYSTEM_NAME)
endif()

include(ExternalProject)

### setup all the necessary paths
if(NOT APP_ROOT AND NOT XBMCROOT)
  set(APP_ROOT ${PROJECT_SOURCE_DIR}/../../..)
elseif(NOT APP_ROOT)
  file(TO_CMAKE_PATH "${XBMCROOT}" APP_ROOT)
else()
  file(TO_CMAKE_PATH "${APP_ROOT}" APP_ROOT)
endif()
get_filename_component(APP_ROOT "${APP_ROOT}" ABSOLUTE)

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

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
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
  # copy the prepare-env.cmake script to the depends path so that we can include it
  file(COPY ${APP_ROOT}/project/cmake/scripts/common/prepare-env.cmake DESTINATION ${DEPENDS_PATH}/lib/kodi)

  # add the location of prepare-env.cmake to CMAKE_MODULE_PATH so that it is found
  list(APPEND CMAKE_MODULE_PATH ${DEPENDS_PATH}/lib/kodi)

  # include prepare-env.cmake which contains the logic to install the addon header bindings etc
  include(prepare-env)
endif()

### get and build all the binary addons
# look for all the addons to be built
file(GLOB_RECURSE addons ${PROJECT_SOURCE_DIR}/addons/*.txt)
foreach(addon ${addons})
  if(NOT (addon MATCHES platforms.txt))
    file(STRINGS ${addon} def)
    separate_arguments(def)
    list(GET def 0 id)

    list(FIND ADDONS_TO_BUILD ${id} idx)
    if(idx GREATER -1 OR ADDONS_TO_BUILD STREQUAL "all")
      get_filename_component(dir ${addon} PATH)
      set(platform_found FALSE)

      # check if the addon has a platforms.txt
      if(EXISTS ${dir}/platforms.txt)
        # get all the specified platforms
        file(STRINGS ${dir}/platforms.txt platforms)
        separate_arguments(platforms)

        # check if the addon should be built for the current platform
        foreach(platform ${platforms})
          if(${platform} STREQUAL "all" OR ${platform} STREQUAL ${CORE_SYSTEM_NAME})
            set(platform_found TRUE)
          endif()
        endforeach()
      else()
        set(platform_found TRUE)
      endif()

      if (${platform_found})
        # make sure the output directory is clean
        if(EXISTS "${CMAKE_INSTALL_PREFIX}/${id}")
          file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/${id}/")
        endif()

        # prepare the setup of the call to externalproject_add()
        set(EXTERNALPROJECT_SETUP INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
                                  CMAKE_ARGS ${BUILD_ARGS})

        # get the URL and revision of the addon
        list(LENGTH def deflength)
        list(GET def 1 url)
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
      endif()
    endif()
  endif()
endforeach()
