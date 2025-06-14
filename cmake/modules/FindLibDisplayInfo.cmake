#.rst:
# FindLibDisplayInfo
# -------
# Finds the libdisplay-info library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibDisplayInfo   - The LibDisplayInfo library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libdisplay-info)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
  else()
    if(LibDisplayInfo_FIND_REQUIRED)
      message(FATAL_ERROR "Libdisplayinfo libraries were not found.")
    endif()
  endif()
endif()
