#.rst:
# FindIso9660
# --------
# Finds the iso9660 library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Iso9660 - The Iso9660 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(Cdio ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} ${SEARCH_QUIET})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  if(TARGET LIBRARY::Cdio)

    set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libiso9660)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

    SETUP_BUILD_VARS()

    SETUP_FIND_SPECS()

    SEARCH_EXISTING_PACKAGES()

    if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
      # Todo: windows?
      if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
        add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      endif()
    endif()
  else()
    include(FindPackageMessage)
    find_package_message(Iso9660pp "Iso9660: Can not find libcdio (REQUIRED)" "")
  endif()
endif()
