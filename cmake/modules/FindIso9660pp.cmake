#.rst:
# FindIso9660pp
# -------------
# Finds the iso9660++ library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Iso9660pp - The Iso9660pp library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(Cdiopp ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} ${SEARCH_QUIET})

  find_package(Iso9660 ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} ${SEARCH_QUIET})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  SETUP_FIND_SPECS()

  if(TARGET LIBRARY::Cdiopp AND TARGET LIBRARY::Iso9660)

    set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libiso9660++)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

    SETUP_BUILD_VARS()

    SETUP_FIND_SPECS()

    SEARCH_EXISTING_PACKAGES()

    if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
      if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
        add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      endif()

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_ISO9660PP)
      ADD_TARGET_COMPILE_DEFINITION()
    endif()
  else()
    include(FindPackageMessage)
    if(NOT TARGET LIBRARY::Cdiopp)
      find_package_message(Iso9660pp "Iso9660pp: Can not find libcdio++ (REQUIRED)" "")
    endif()
    if(NOT TARGET LIBRARY::Iso9660)
      find_package_message(Iso9660pp "Iso9660pp: Can not find Iso9660 (REQUIRED)" "")
    endif()
  endif()
endif()
