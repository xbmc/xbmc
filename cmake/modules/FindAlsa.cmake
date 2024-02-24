#.rst:
# FindAlsa
# --------
# Finds the Alsa library
#
# This will define the following target:
#
#   ALSA::ALSA   - The Alsa library

if(NOT TARGET ALSA::ALSA)
  find_package(PkgConfig)

  if(Alsa_FIND_VERSION)
    if(Alsa_FIND_VERSION_EXACT)
      set(Alsa_FIND_SPEC "=${Alsa_FIND_VERSION_COMPLETE}")
    else()
      set(Alsa_FIND_SPEC ">=${Alsa_FIND_VERSION_COMPLETE}")
    endif()
  endif()

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_ALSA alsa${Alsa_FIND_SPEC} QUIET)
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
    add_library(ALSA::ALSA UNKNOWN IMPORTED)
    set_target_properties(ALSA::ALSA PROPERTIES
                                     IMPORTED_LOCATION "${ALSA_LIBRARY}"
                                     INTERFACE_COMPILE_DEFINITIONS HAS_ALSA=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP ALSA::ALSA)
  endif()
endif()
