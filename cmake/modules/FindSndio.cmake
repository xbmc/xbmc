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
  find_path(SNDIO_INCLUDE_DIR sndio.h)
  find_library(SNDIO_LIBRARY sndio)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Sndio
                                    REQUIRED_VARS SNDIO_LIBRARY SNDIO_INCLUDE_DIR)

  if(SNDIO_FOUND)
    list(APPEND AUDIO_BACKENDS_LIST "sndio")
    set(AUDIO_BACKENDS_LIST ${AUDIO_BACKENDS_LIST} PARENT_SCOPE)

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${SNDIO_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${SNDIO_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAS_SNDIO)
  endif()
endif()
