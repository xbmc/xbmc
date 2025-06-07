#
# FindSndio
# ---------
# Finds the Sndio Library
#
# This will define the following target:
#
#  ${APP_NAME_LC}::Sndio - the sndio library
#
if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC sndio)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    list(APPEND AUDIO_BACKENDS_LIST "sndio")
    set(AUDIO_BACKENDS_LIST ${AUDIO_BACKENDS_LIST} PARENT_SCOPE)

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_SNDIO)
    ADD_TARGET_COMPILE_DEFINITION()
  endif()
endif()
