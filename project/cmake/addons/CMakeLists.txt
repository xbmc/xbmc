cmake_minimum_required(VERSION 3.1)
project(kodi-addons)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

option(ADDON_TARBALL_CACHING "Cache downloaded addon source tarballs?" ON)
if(ADDON_TARBALL_CACHING)
  message(STATUS "Addon source tarball caching is enabled")
else()
  message(STATUS "Addon source tarball caching is disabled")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT CORE_SYSTEM_NAME)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CORE_SYSTEM_NAME "osx")
  else()
    string(TOLOWER ${CMAKE_SYSTEM_NAME} CORE_SYSTEM_NAME)
  endif()
endif()

include(ExternalProject)

### setup all the necessary paths
if(APP_ROOT)
  set(CORE_SOURCE_DIR ${APP_ROOT})
  unset(APP_ROOT)
  message(WARNING "APP_ROOT is deprecated. Please use CORE_SOURCE_DIR instead.")
endif()
if(NOT CORE_SOURCE_DIR)
  set(CORE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../..)
else()
  file(TO_CMAKE_PATH "${CORE_SOURCE_DIR}" CORE_SOURCE_DIR)
endif()
get_filename_component(CORE_SOURCE_DIR "${CORE_SOURCE_DIR}" ABSOLUTE)

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
get_filename_component(DEPENDS_PATH "${DEPENDS_PATH}" ABSOLUTE)

if(NOT PLATFORM_DIR)
  set(PLATFORM_DIR ${CORE_SOURCE_DIR}/project/cmake/platform/${CORE_SYSTEM_NAME})
  file(TO_CMAKE_PATH "${PLATFORM_DIR}" PLATFORM_DIR)
endif()

# make sure CMAKE_PREFIX_PATH is set
if(NOT CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH "${DEPENDS_PATH}")
else()
  file(TO_CMAKE_PATH "${CMAKE_PREFIX_PATH}" CMAKE_PREFIX_PATH)
  list(APPEND CMAKE_PREFIX_PATH "${DEPENDS_PATH}")
endif()

# check for autoconf stuff to pass on
if(AUTOCONF_FILES)
  separate_arguments(AUTOCONF_FILES)
  set(CROSS_AUTOCONF "yes")
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/output/addons")
endif()
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

set(BUILD_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
               -DPACKAGE_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig
               -DDEPENDS_PATH=${DEPENDS_PATH}
               -DOVERRIDE_PATHS=${OVERRIDE_PATHS}
               -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
               -DCMAKE_USER_MAKE_RULES_OVERRIDE=${CMAKE_USER_MAKE_RULES_OVERRIDE}
               -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=${CMAKE_USER_MAKE_RULES_OVERRIDE_CXX}
               -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
               -DBUILD_SHARED_LIBS=1
               -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
               -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS})

if(MSVC)
  # move cmake specific targets to a CMakePredefinedTargets folder in Visual Studio
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

option(PACKAGE_ZIP "Prepare built addons for packaging" OFF)
if(PACKAGE_ZIP)
  # needed for project installing
  list(APPEND BUILD_ARGS -DPACKAGE_ZIP=ON)

  # figure out where to store the packaged ZIP archives
  if(NOT PACKAGE_DIR)
    set(PACKAGE_DIR "${BUILD_DIR}/zips")
  else()
    file(TO_CMAKE_PATH "${PACKAGE_DIR}" PACKAGE_DIR)
  endif()
  list(APPEND BUILD_ARGS -DPACKAGE_DIR=${PACKAGE_DIR})

  message(STATUS "ZIP packaging enabled (destination: ${PACKAGE_DIR})")
endif()

if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND BUILD_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
  message(STATUS "Toolchain specified")
  message(STATUS ${BUILD_ARGS})
endif()

if(NOT ADDONS_TO_BUILD)
  set(ADDONS_TO_BUILD "all")
else()
  string(STRIP "${ADDONS_TO_BUILD}" ADDONS_TO_BUILD)
  message(STATUS "Building following addons: ${ADDONS_TO_BUILD}")
  separate_arguments(ADDONS_TO_BUILD)
endif()

if(NOT ADDONS_DEFINITION_DIR)
  set(ADDONS_DEFINITION_DIR ${PROJECT_SOURCE_DIR}/addons)
else()
  file(TO_CMAKE_PATH "${ADDONS_DEFINITION_DIR}" ADDONS_DEFINITION_DIR)
endif()
get_filename_component(ADDONS_DEFINITION_DIR "${ADDONS_DEFINITION_DIR}" ABSOLUTE)

if(ADDON_SRC_PREFIX)
  if(NOT IS_ABSOLUTE ${ADDON_SRC_PREFIX})
    get_filename_component(ADDON_SRC_PREFIX "${CMAKE_BINARY_DIR}/${ADDON_SRC_PREFIX}" ABSOLUTE)
  endif()
  message(STATUS "Overriding addon source directory prefix: ${ADDON_SRC_PREFIX}")
endif()

if(NOT APP_LIB_DIR)
  set(APP_LIB_DIR "${DEPENDS_PATH}/lib/kodi")
else()
  file(TO_CMAKE_PATH "${APP_LIB_DIR}" APP_LIB_DIR)
endif()

set(APP_PREFIX "${CMAKE_INSTALL_PREFIX}")

# check for platform specific stuff
if(EXISTS ${PLATFORM_DIR}/defines.txt)
  file(STRINGS ${PLATFORM_DIR}/defines.txt platformdefines)

  if(NOT ARCH_DEFINES AND platformdefines)
    set(ARCH_DEFINES ${platformdefines})
  endif()
endif()

# include check_target_platform() function
include(${CORE_SOURCE_DIR}/project/cmake/scripts/common/CheckTargetPlatform.cmake)

set(ADDON_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
if(NOT WIN32)
  # check install permissions
  check_install_permissions(${CMAKE_INSTALL_PREFIX} can_write)
  if(NOT ${can_write} AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(NEED_SUDO TRUE)
    set(ADDON_INSTALL_DIR ${CMAKE_BINARY_DIR}/.install)
    message(STATUS "NEED_SUDO: ${NEED_SUDO}")
  endif()
endif()

### prepare the build environment for the binary addons
# copy the PrepareEnv.cmake script to the depends path so that we can include it
file(COPY ${CORE_SOURCE_DIR}/project/cmake/scripts/common/PrepareEnv.cmake DESTINATION ${APP_LIB_DIR})

# add the location of PrepareEnv.cmake to CMAKE_MODULE_PATH so that it is found
list(APPEND CMAKE_MODULE_PATH ${APP_LIB_DIR})

# include PrepareEnv.cmake which contains the logic to install the addon header bindings etc
include(PrepareEnv)

### add the depends subdirectory for any general dependencies
message(STATUS "\n-- ---- Preparing general dependencies ----")
add_subdirectory(depends)

# add a custom target "package-addons" which will package and install all addons
add_custom_target(package-addons)

### get and build all the binary addons
# look for all the addons to be built
file(GLOB_RECURSE addons ${ADDONS_DEFINITION_DIR}/*.txt)

#if there are no addons assume that bootstrapping hasn't happened yet
if(NOT addons)
  message(STATUS "Bootstrapping all default repositories as no addons were found...")
  set(BOOTSTRAP_BUILD_DIR "${BUILD_DIR}/bootstrap")

  # make sure that the bootstraps build addon exists
  if(NOT EXISTS ${BOOTSTRAP_BUILD_DIR})
    file(MAKE_DIRECTORY ${BOOTSTRAP_BUILD_DIR})
  endif()

  string(REPLACE ";" " " ADDONS_TO_BUILD_STR "${ADDONS_TO_BUILD}")
  # generate the bootstrap buildsystem
  execute_process(COMMAND ${CMAKE_COMMAND} ${PROJECT_SOURCE_DIR}/bootstrap
                                           -DCMAKE_INSTALL_PREFIX:PATH=${ADDONS_DEFINITION_DIR}
                                           -DBUILD_DIR:PATH=${BOOTSTRAP_BUILD_DIR}
                                           -DADDONS_TO_BUILD:STRING=${ADDONS_TO_BUILD_STR}
                  WORKING_DIRECTORY ${BOOTSTRAP_BUILD_DIR})

  # execute the generated bootstrap buildsystem
  execute_process(COMMAND ${CMAKE_COMMAND} --build ${BOOTSTRAP_BUILD_DIR}
                  WORKING_DIRECTORY ${BOOTSTRAP_BUILD_DIR})

  # now look for all the addons to be built again
  file(GLOB_RECURSE addons ${ADDONS_DEFINITION_DIR}/*.txt)

  if(NOT addons)
    message(FATAL_ERROR "No addons available to be built")
  endif()
endif()

# Track if at least one addon has been found. Everything else is likely an
# error either in ADDONS_TO_BUILD or in the directory configuration.
set(SUPPORTED_ADDON_FOUND FALSE)

foreach(addon ${addons})
  if(NOT (addon MATCHES platforms.txt))
    file(STRINGS ${addon} def)
    separate_arguments(def)
    list(GET def 0 id)

    set(ADDON_FOUND FALSE)
    # try to find a perfect match
    list(FIND ADDONS_TO_BUILD ${id} idx)
    if(idx GREATER -1 OR "${ADDONS_TO_BUILD}" STREQUAL "all")
      set(ADDON_FOUND TRUE)
    # Maybe we have a regex
    elseif(id MATCHES "${ADDONS_TO_BUILD}")
      message(STATUS "Pattern ${ADDONS_TO_BUILD} matches ${id}, building addon")
      set(ADDON_FOUND TRUE)
    endif()

    if(ADDON_FOUND)
      message(STATUS "\n-- ---- Configuring addon ${addon} ----")
      set(SUPPORTED_ADDON_FOUND TRUE)

      get_filename_component(dir ${addon} PATH)

      # check if the addon has a platforms.txt
      set(platform_found FALSE)
      check_target_platform(${dir} ${CORE_SYSTEM_NAME} platform_found)

      if(${platform_found})
        # make sure the output directory is clean
        if(EXISTS "${CMAKE_INSTALL_PREFIX}/${id}")
          file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/${id}/")
        endif()

        # get the URL and revision of the addon
        list(LENGTH def deflength)
        list(GET def 1 url)

        set(archive_name ${id})
        if(ADDON_SRC_PREFIX)
          set(SOURCE_DIR ${ADDON_SRC_PREFIX}/${id})
          set(archive_name "")
        else()
          set(SOURCE_DIR "")
        endif()

        # if there is a 3rd parameter in the file, we consider it a git revision
        if(deflength GREATER 2 AND "${SOURCE_DIR}" STREQUAL "")
          list(GET def 2 revision)

          # we need access to a git executable
          find_package(Git REQUIRED)

          # resolve revision to git hash
          execute_process(COMMAND ${GIT_EXECUTABLE} ls-remote ${url} ${revision} OUTPUT_VARIABLE revision_hash)
          # git ls-remote only works on branches and tag names but not on revisions
          if(NOT "${revision_hash}" STREQUAL "")
            string(REPLACE "\t" ";" revision_list ${revision_hash})
            list(GET revision_list 0 revision_hash)
            message(STATUS "${id}: git branch/tag ${revision} resolved to hash: ${revision_hash}")
            set(revision ${revision_hash})
          endif()

          # Note: downloading specific revisions via http in the format below is probably github specific
          # if we ever use other repositories, this might need adapting
          set(url ${url}/archive/${revision}.tar.gz)
          set(archive_name ${archive_name}-${revision})
        elseif("${SOURCE_DIR}" STREQUAL "")
          # check if the URL starts with file://
          string(REGEX MATCH "^file://.*$" local_url "${url}")

          #if not we assume this to be a local directory
          if(local_url)
            # this is not an archive
            set(archive_name "")

            # remove the file:// protocol from the URL
            string(REPLACE "file://" "" SOURCE_DIR "${url}")

            # on win32 we may have to remove another leading /
            if(WIN32)
              # check if the path is a local path
              string(REGEX MATCH "^/.*$" local_path "${SOURCE_DIR}")
              if(local_path)
                string(SUBSTRING "${SOURCE_DIR}" 1 -1 SOURCE_DIR)
              endif()
            endif()
          endif()
        endif()

        # download the addon if necessary
        if(NOT "${archive_name}" STREQUAL "")
          # download and extract the addon
          if(NOT ADDON_TARBALL_CACHING OR NOT EXISTS ${BUILD_DIR}/download/${archive_name}.tar.gz)
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
            message(FATAL_ERROR "${id}: error extracting ${BUILD_DIR}/download/${archive_name}.tar.gz")
          else()
            file(RENAME "${extract_dir}" "${BUILD_DIR}/${id}")
          endif()

          set(SOURCE_DIR ${BUILD_DIR}/${id})
        endif()

        if(NOT "${SOURCE_DIR}" STREQUAL "" AND EXISTS ${SOURCE_DIR})
          # create a list of addons we are building
          list(APPEND ALL_ADDONS_BUILDING ${id})

          # setup the buildsystem for the addon
          externalproject_add(${id}
                              SOURCE_DIR ${SOURCE_DIR}
                              INSTALL_DIR ${ADDON_INSTALL_DIR}
                              CMAKE_ARGS ${BUILD_ARGS})

          # add a custom step to the external project between the configure and the build step which will always
          # be executed and therefore forces a re-build of all changed files
          externalproject_add_step(${id} forcebuild
                                   COMMAND ${CMAKE_COMMAND} -E echo "Force build of ${id}"
                                   DEPENDEES configure
                                   DEPENDERS build
                                   ALWAYS 1)

          # add "kodi-platform" as a dependency to every addon
          add_dependencies(${id} kodi-platform)

          set(${id}_DEPENDS_DIR ${SOURCE_DIR}/depends)

          if(EXISTS ${${id}_DEPENDS_DIR})
            include(${CORE_SOURCE_DIR}/project/cmake/scripts/common/HandleDepends.cmake)
            add_addon_depends(${id} ${${id}_DEPENDS_DIR})
            if(${id}_DEPS AND NOT "${${id}_DEPS}" STREQUAL "")
              message(STATUS "${id} DEPENDENCIES: ${${id}_DEPS}")
              add_dependencies(${id} ${${id}_DEPS})
            endif()
          endif()

          if(CROSS_AUTOCONF AND AUTOCONF_FILES)
            if(EXISTS ${SOURCE_DIR}/bootstrap/autoreconf.txt)
              file(STRINGS ${SOURCE_DIR}/bootstrap/autoreconf.txt conf_dirs)
              foreach(conf_dir ${conf_dirs})
                foreach(afile ${AUTOCONF_FILES})
                  message(STATUS "copying ${afile} to ${SOURCE_DIR}/${conf_dir}")
                  file(COPY ${afile} DESTINATION ${SOURCE_DIR}/${conf_dir})
                endforeach()
              endforeach()
            endif()
          endif()

          # create a forwarding target to the addon-package target
          add_custom_target(package-${id}
                    COMMAND ${CMAKE_COMMAND} --build ${id}-prefix/src/${id}-build --target addon-package
                    DEPENDS ${id})
          add_dependencies(package-addons package-${id})

        else()
          message(FATAL_ERROR "${id}: invalid or missing addon source directory at ${SOURCE_DIR}")
        endif()
      else()
        # add a dummy target for addons that are unsupported on this platform
        add_custom_target(${id} COMMAND ${CMAKE_COMMAND} -E echo "IGNORED ${id} - not supported on ${CORE_SYSTEM_NAME}\n")
      endif()
    endif()
  endif()
endforeach()
message(STATUS "")

if(NEED_SUDO)
  add_custom_target(install
                    COMMAND ${CMAKE_COMMAND} -E echo "\n\n"
                    COMMAND ${CMAKE_COMMAND} -E echo "WARNING: sudo rights needed to install to ${CMAKE_INSTALL_PREFIX}\n"
                    COMMAND sudo ${CMAKE_COMMAND} -E copy_directory ${ADDON_INSTALL_DIR}/ ${CMAKE_INSTALL_PREFIX}/
                    COMMAND sudo ${CMAKE_COMMAND} -E remove_directory ${ADDON_INSTALL_DIR}/
                    COMMAND sudo -k)
endif()

if(NOT SUPPORTED_ADDON_FOUND)
  message(FATAL_ERROR "${ADDONS_TO_BUILD} did not match any of the supported addons. \
                       A list of supported addons can be viewed by building the 'supported_addons' target. \
                       Addon definitions are loaded from ADDONS_DEFINITION_DIR (${ADDONS_DEFINITION_DIR}).")
endif()

# add custom target "supported_addons" that returns all addons that are supported on this platform
string(REPLACE ";" " " ALL_ADDONS_BUILDING "${ALL_ADDONS_BUILDING}")
add_custom_target(supported_addons COMMAND ${CMAKE_COMMAND} -E echo "ALL_ADDONS_BUILDING: ${ALL_ADDONS_BUILDING}" VERBATIM)
