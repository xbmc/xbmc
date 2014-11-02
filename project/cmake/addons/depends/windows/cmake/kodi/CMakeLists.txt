project(kodi)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

### setup all the necessary paths
if(NOT APP_ROOT AND NOT XBMCROOT)
  set(APP_ROOT ${PROJECT_SOURCE_DIR}/../../../../../../..)
elseif(NOT APP_ROOT)
  file(TO_CMAKE_PATH "${XBMCROOT}" APP_ROOT)
else()
  file(TO_CMAKE_PATH "${APP_ROOT}" APP_ROOT)
endif()
get_filename_component(APP_ROOT "${APP_ROOT}" ABSOLUTE)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
  message(FATAL_ERROR "CMAKE_INSTALL_PREFIX (${CMAKE_INSTALL_PREFIX}) is not a valid target directory.")
else()
  file(TO_CMAKE_PATH "${CMAKE_INSTALL_PREFIX}" CMAKE_INSTALL_PREFIX)
endif()
get_filename_component(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)

### prepare the environment
# set the DEPENDS_PATH variable used by prepare-env
set(DEPENDS_PATH "${CMAKE_INSTALL_PREFIX}")

# copy the prepare-env.cmake script so that we can include it
file(COPY ${APP_ROOT}/project/cmake/scripts/common/prepare-env.cmake DESTINATION ${CMAKE_BINARY_DIR})

# include prepare-env.cmake which contains the logic to install the addon header bindings etc
include(prepare-env)