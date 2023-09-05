#
# FindSndio
# ---------
# Finds the Sndio Library
#
# This will define the following target:
#
#  Sndio::Sndio - the sndio library
#
if(NOT TARGET Sndio::Sndio)
  find_path(SNDIO_INCLUDE_DIR sndio.h NO_CACHE)
  find_library(SNDIO_LIBRARY sndio NO_CACHE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Sndio
                                    REQUIRED_VARS SNDIO_LIBRARY SNDIO_INCLUDE_DIR)

  if(SNDIO_FOUND)
    list(APPEND AUDIO_BACKENDS_LIST "sndio")
    set(AUDIO_BACKENDS_LIST ${AUDIO_BACKENDS_LIST} PARENT_SCOPE)


    add_library(Sndio::Sndio UNKNOWN IMPORTED)
    set_target_properties(Sndio::Sndio PROPERTIES
                                       IMPORTED_LOCATION "${SNDIO_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${SNDIO_INCLUDE_DIR}"
                                       INTERFACE_COMPILE_DEFINITIONS HAS_SNDIO=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Sndio::Sndio)
  endif()
endif()
