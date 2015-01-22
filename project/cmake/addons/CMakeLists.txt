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

if(NOT BUILD_DIR)
    set(BUILD_DIR "${CMAKE_BINARY_DIR}/build")
else()
  file(TO_CMAKE_PATH "${BUILD_DIR}" BUILD_DIR)
endif()
get_filename_component(BUILD_DIR "${BUILD_DIR}" ABSOLUTE)

if(NOT DEPENDS_PATH)
  set(DEPENDS_PATH "${BUILD_DIR}/depends")
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

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/output/addons")
endif()
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

set(BUILD_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
               -DPACKAGE_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig
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

add_subdirectory(depends)

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
          else()
            # check if the platform is defined as "!<platform>"
            string(SUBSTRING ${platform} 0 1 platform_first)
            if(${platform_first} STREQUAL "!")
              # extract the platform
              string(LENGTH ${platform} platform_length)
              MATH(EXPR platform_length "${platform_length} - 1")
              string(SUBSTRING ${platform} 1 ${platform_length} platform)

              # check if the current platform does not match the extracted platform
              if (NOT ${platform} STREQUAL ${CORE_SYSTEM_NAME})
                set(platform_found TRUE)
              endif()
            endif()
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

        # get the URL and revision of the addon
        list(LENGTH def deflength)
        list(GET def 1 url)

        set(archive_name ${id})

          # if there is a 3rd parameter in the file, we consider it a git revision
        if(deflength GREATER 2)
          list(GET def 2 revision)

          # Note: downloading specific revisions via http in the format below is probably github specific
          # if we ever use other repositories, this might need adapting
          set(url ${url}/archive/${revision}.tar.gz)
          set(archive_name ${archive_name}-${revision})
        endif()

        # download and extract the addon
        if(NOT EXISTS ${BUILD_DIR}/download/${archive_name}.tar.gz)
          # cleanup any of the previously downloaded archives of this addon
          file(GLOB archives "${BUILD_DIR}/download/${id}*.tar.gz")
          if(archives)
            message(STATUS "Removing old archives of ${id}: ${archives}")
            file(REMOVE ${archives})
          endif()

          # download the addon
          file(DOWNLOAD "${url}" "${BUILD_DIR}/download/${archive_name}.tar.gz" STATUS dlstatus LOG dllog SHOW_PROGRESS)
          list(GET dlstatus 0 retcode)
          if(NOT ${retcode} EQUAL 0)
            message(FATAL_ERROR "ERROR downloading ${url} - status: ${dlstatus} log: ${dllog}")
          endif()
        endif()

        # remove any previously extracted version of the addon
        if(EXISTS "${BUILD_DIR}/${id}")
          file(REMOVE_RECURSE "${BUILD_DIR}/${id}")
        endif()

        # extract the addon from the archive
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzvf ${BUILD_DIR}/download/${archive_name}.tar.gz
                        WORKING_DIRECTORY ${BUILD_DIR})
        file(GLOB extract_dir "${BUILD_DIR}/${archive_name}*")
        if(extract_dir STREQUAL "")
          message(FATAL_ERROR "Error extracting ${BUILD_DIR}/download/${archive_name}.tar.gz")
        else()
          file(RENAME "${extract_dir}" "${BUILD_DIR}/${id}")
        endif()

        list(APPEND downloaded_addons ${id})

      endif()
    endif()
  endif()
endforeach()

foreach(id ${downloaded_addons})
  externalproject_add(${id}
                      SOURCE_DIR ${BUILD_DIR}/${id}
                      INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
                      CMAKE_ARGS ${BUILD_ARGS})

  # on win32 we need to add the "kodi" dependency to every addon
  if (WIN32)
    add_dependencies(${id} kodi)
  endif()

  set(${id}_DEPENDS_DIR ${BUILD_DIR}/${id}/depends)

  if(EXISTS ${${id}_DEPENDS_DIR})
    include(${APP_ROOT}/project/cmake/scripts/common/handle-depends.cmake)
    add_addon_depends(${id} ${${id}_DEPENDS_DIR})
    if (${id}_DEPS AND NOT "${${id}_DEPS}" STREQUAL "")
      message(STATUS "${id} DEPENDENCIES: ${${id}_DEPS}")
      add_dependencies(${id} ${${id}_DEPS})
    endif()
  endif()
endforeach()
