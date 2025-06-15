#.rst:
# FindLzo2
# --------
# Finds the Lzo2 library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Lzo2   - The Lzo2 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC lzo2)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    elseif(TARGET lzo2::lzo2)
      # Kodi target - windows prebuilt lib
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS lzo2::lzo2)
    endif()
  else()
    if(LibLzo2_FIND_REQUIRED)
      message(FATAL_ERROR "Lzo2 library was not found.")
    endif()
  endif()
endif()
