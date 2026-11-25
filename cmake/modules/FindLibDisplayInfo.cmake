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

    # The HDMI Forum data blocks, which hold the variable refresh rate range, came in 0.3.0
    if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_GREATER_EQUAL 0.3.0)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_LIBDISPLAYINFO_HDMI_FORUM)
      ADD_TARGET_COMPILE_DEFINITION()
    endif()
  else()
    if(LibDisplayInfo_FIND_REQUIRED)
      message(FATAL_ERROR "Libdisplayinfo libraries were not found.")
    endif()
  endif()
endif()
