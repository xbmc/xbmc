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

if(NOT DEPENDS_PATH)
  set(DEPENDS_PATH "${CMAKE_BINARY_DIR}/addons")
else()
  file(TO_CMAKE_PATH "${DEPENDS_PATH}" DEPENDS_PATH)
endif()
# xbmc-config.cmake.in (further down) expects a "prefix" variable
get_filename_component(prefix "${DEPENDS_PATH}" ABSOLUTE)

#make sure CMAKE_PREFIX_PATH is set
if(NOT CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH "${DEPENDS_PATH}")
else()
  file(TO_CMAKE_PATH "${CMAKE_PREFIX_PATH}" CMAKE_PREFIX_PATH)
  list(APPEND CMAKE_PREFIX_PATH "${DEPENDS_PATH}")
endif()

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${prefix})
endif()
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

set(BUILD_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
               -DCMAKE_BUILD_TYPE=Release
               -DCMAKE_USER_MAKE_RULES_OVERRIDE=${CMAKE_USER_MAKE_RULES_OVERRIDE}
               -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=${CMAKE_USER_MAKE_RULES_OVERRIDE_CXX}
               -DBUILD_SHARED_LIBS=1)

if(PACKAGE_ZIP)
  #needed for project installing
  list(APPEND BUILD_ARGS -DPACKAGE_ZIP=1)
  MESSAGE("package zip specified")
endif()

if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND BUILD_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
  MESSAGE("toolchain specified")
  MESSAGE(${BUILD_ARGS})
endif()

# parse version.txt to get the version info
if (EXISTS "${XBMCROOT}/version.txt")
  file(STRINGS "${XBMCROOT}/version.txt" versions)
  foreach (version ${versions})
    string(REGEX MATCH "^[^ ]+" version_name ${version})
    string(REPLACE "${version_name} " "" version_value ${version})
    set(APP_${version_name} "${version_value}")
  endforeach()
endif()

# bail if we can't parse versions
if (NOT DEFINED APP_VERSION_MAJOR OR NOT DEFINED APP_VERSION_MINOR)
  message(FATAL_ERROR "Could not determine app version! make sure that ${XBMCROOT}/version.txt exists")
endif()

### copy all the addon binding header files to include/xbmc
# make sure include/xbmc exists and is empty
set(XBMC_LIB_DIR ${DEPENDS_PATH}/lib/xbmc)
if(NOT EXISTS "${XBMC_LIB_DIR}/")
  file(MAKE_DIRECTORY ${XBMC_LIB_DIR})
endif()

set(XBMC_INCLUDE_DIR ${DEPENDS_PATH}/include/xbmc)
if(NOT EXISTS "${XBMC_INCLUDE_DIR}/")
  file(MAKE_DIRECTORY ${XBMC_INCLUDE_DIR})
endif()

# generate the proper xbmc-config.cmake file
configure_file(${XBMCROOT}/project/cmake/xbmc-config.cmake.in ${XBMC_LIB_DIR}/xbmc-config.cmake @ONLY)
# copy cmake helpers to lib/xbmc
file(COPY ${XBMCROOT}/project/cmake/xbmc-addon-helpers.cmake ${XBMCROOT}/project/cmake/AddOptions.cmake DESTINATION ${XBMC_LIB_DIR})

### copy all the addon binding header files to include/xbmc
# parse addon-bindings.mk to get the list of header files to copy
file(STRINGS ${XBMCROOT}/xbmc/addons/addon-bindings.mk bindings)
string(REPLACE "\n" ";" bindings "${bindings}")
foreach(binding ${bindings})
  string(REPLACE " =" ";" binding "${binding}")
  string(REPLACE "+=" ";" binding "${binding}")
  list(GET binding 1 header)
  # copy the header file to include/xbmc
  file(COPY ${XBMCROOT}/${header} DESTINATION ${XBMC_INCLUDE_DIR})
endforeach()


### get and build all the binary addons
# look for all the addons to be built
file(GLOB addons ${PROJECT_SOURCE_DIR}/addons/*.txt)
foreach(addon ${addons})
  file(STRINGS ${addon} def)
  string(REPLACE " " ";" def ${def})
  list(GET def 0 id)
  list(GET def 1 url)
  list(GET def 2 revision)

  # make sure the output directory is clean
  if(EXISTS "${CMAKE_INSTALL_PREFIX}/${id}")
    file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/${id}/")
  endif()

  # add the addon as an external project for automatic building
  externalproject_add(${id}
                      GIT_REPOSITORY ${url}/${id}
                      GIT_TAG ${revision}
                      INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
                      CMAKE_ARGS ${BUILD_ARGS}
                     )
endforeach()
