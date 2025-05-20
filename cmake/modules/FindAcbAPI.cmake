#.rst:
# FindAcbAPI
# --------
# Finds the AcbAPI library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::AcbAPI   - The acbAPI library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libAcbAPI)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(libAcbAPI_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})

    # creates an empty library to install on webOS 5+ devices
    file(TOUCH dummy.c)
    add_library(AcbAPI SHARED dummy.c)
    set_target_properties(AcbAPI PROPERTIES VERSION 1.0.0 SOVERSION 1)
  else()
    if(AcbAPI_FIND_REQUIRED)
      message(FATAL_ERROR "AcbAPI libraries were not found.")
    endif()
  endif()
endif()
