#.rst:
# FindCdiopp
# ----------
# Finds the cdio++ library
#
# This will define the following target:
#
# ${APP_NAME_LC}::Cdiopp - The Cdio++ library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(Cdio ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} ${SEARCH_QUIET})

  if(TARGET ${APP_NAME_LC}::Cdio)
    include(cmake/scripts/common/ModuleHelpers.cmake)
  
    set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libcdio++)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)
  
    SETUP_BUILD_VARS()
  
    SETUP_FIND_SPECS()
  
    SEARCH_EXISTING_PACKAGES()
  
    if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    endif()
  else()
    include(FindPackageMessage)
    find_package_message(Cdiopp "Cdiopp: Can not find libcdio (REQUIRED)" "")
  endif()
endif()
