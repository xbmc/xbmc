#.rst:
# FindAlsa
# --------
# Finds the Alsa library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Alsa   - The Alsa library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  SETUP_FIND_SPECS()

  find_package(PkgConfig ${SEARCH_QUIET})

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_ALSA alsa${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET})
  endif()

  find_path(ALSA_INCLUDE_DIR NAMES alsa/asoundlib.h
                             HINTS ${PC_ALSA_INCLUDEDIR}
                             NO_CACHE)
  find_library(ALSA_LIBRARY NAMES asound
                            HINTS ${PC_ALSA_LIBDIR}
                            NO_CACHE)

  set(ALSA_VERSION ${PC_ALSA_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Alsa
                                    REQUIRED_VARS ALSA_LIBRARY ALSA_INCLUDE_DIR
                                    VERSION_VAR ALSA_VERSION)

  if(ALSA_FOUND)
    list(APPEND AUDIO_BACKENDS_LIST "alsa")
    set(AUDIO_BACKENDS_LIST ${AUDIO_BACKENDS_LIST} PARENT_SCOPE)

    # We explicitly dont include ALSA_INCLUDE_DIR, as 'timer.h' is a dangerous file
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${ALSA_LIBRARY}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAS_ALSA)
  endif()
endif()
