project(xbmc)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

### setup all the necessary paths
if(NOT XBMCROOT)
  set(XBMCROOT ${PROJECT_SOURCE_DIR}/../../../../../../..)
else()
  file(TO_CMAKE_PATH "${XBMCROOT}" XBMCROOT)
endif()
get_filename_component(XBMCROOT "${XBMCROOT}" ABSOLUTE)

if(NOT CMAKE_INSTALL_PREFIX)
  message(FATAL_ERROR "CMAKE_INSTALL_PREFIX (${CMAKE_INSTALL_PREFIX}) is not a valid target directory.")
else()
  file(TO_CMAKE_PATH "${CMAKE_INSTALL_PREFIX}" CMAKE_INSTALL_PREFIX)
endif()
get_filename_component(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)

### prepare the environment
# set the DEPENDS_PATH variable used by xbmc-prepare-env
set(DEPENDS_PATH "${CMAKE_INSTALL_PREFIX}")

# copy the xbmc-prepare-env.cmake script so that we can include it
file(COPY ${XBMCROOT}/project/cmake/scripts/common/xbmc-prepare-env.cmake DESTINATION ${CMAKE_BINARY_DIR})

# include xbmc-prepare-env.cmake which contains the logic to install the addon header bindings etc
include(xbmc-prepare-env)