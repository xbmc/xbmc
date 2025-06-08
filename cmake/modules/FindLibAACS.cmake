#.rst:
# FindLibAACS
# ----------
# Finds the libaacs library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibAACS   - The libaacs library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libaacs)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  # Kodi libaacs-0.9.0-* prebuilt libs have a broken cmake config file
  # We need to detect this, otherwise the call to find_package will run and error out
  # if we detect it, make the change to correct in the source config file
  if(EXISTS ${DEPENDS_PATH}/lib/cmake/libaacs/libaacs-config.cmake)
    file(READ ${DEPENDS_PATH}/lib/cmake/libaacs/libaacs-config.cmake aacs_config_output)
    string(FIND ${aacs_config_output} "libbdplus.cmake" BROKEN_CONFIG)

    if(${BROKEN_CONFIG} GREATER "-1")
      string(REPLACE "libbdplus.cmake" "libaacs.cmake" aacs_config_output ${aacs_config_output})
      file(WRITE ${DEPENDS_PATH}/lib/cmake/libaacs/libaacs-config.cmake ${aacs_config_output})
    endif()
  endif()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    elseif(TARGET libaacs::libaacs)
      # Kodi target - windows prebuilt lib
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libaacs::libaacs)
    endif()
  endif()
endif()
