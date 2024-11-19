#.rst:
# FindPipewire
# --------------
# Finds the Pipewire library
#
# This will define the following variables::
#
#  PIPEWIRE_FOUND - system has the Pipewire library
#  PIPEWIRE_INCLUDE_DIRS - the Pipewire include directory
#  PIPEWIRE_LIBRARIES - the libraries needed to use Pipewire
#  PIPEWIRE_DEFINITIONS - the definitions needed to use Pipewire
#

if(Pipewire_FIND_VERSION)
  if(Pipewire_FIND_VERSION_EXACT)
    set(Pipewire_FIND_SPEC "=${Pipewire_FIND_VERSION_COMPLETE}")
  else()
    set(Pipewire_FIND_SPEC ">=${Pipewire_FIND_VERSION_COMPLETE}")
  endif()
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PIPEWIRE libpipewire-0.3${Pipewire_FIND_SPEC} QUIET)
  pkg_check_modules(PC_SPA libspa-0.2>=0.2 QUIET)
endif()

find_path(PIPEWIRE_INCLUDE_DIR NAMES pipewire/pipewire.h
                               HINTS ${PC_PIPEWIRE_INCLUDEDIR}
                               PATH_SUFFIXES pipewire-0.3)

find_path(SPA_INCLUDE_DIR NAMES spa/support/plugin.h
                          HINTS ${PC_SPA_INCLUDEDIR}
                          PATH_SUFFIXES spa-0.2)

find_library(PIPEWIRE_LIBRARY NAMES pipewire-0.3
                              HITNS ${PC_PIPEWIRE_LIBDIR})

if(PC_PIPEWIRE_VERSION)
  set(PIPEWIRE_VERSION_STRING ${PC_PIPEWIRE_VERSION})
elseif(PIPEWIRE_INCLUDE_DIR AND EXISTS ${PIPEWIRE_INCLUDE_DIR}/pipewire/version.h)
  file(STRINGS ${PIPEWIRE_INCLUDE_DIR}/pipewire/version.h PIPEWIRE_STRINGS)
  string(REGEX MATCH "#define PW_MAJOR \([0-9]+\)" MAJOR_VERSION "${PIPEWIRE_STRINGS}")
  set(MAJOR_VERSION ${CMAKE_MATCH_1})
  string(REGEX MATCH "#define PW_MINOR \([0-9]+\)" MINOR_VERSION "${PIPEWIRE_STRINGS}")
  set(MINOR_VERSION ${CMAKE_MATCH_1})
  string(REGEX MATCH "#define PW_MICRO \([0-9]+\)" MICRO_VERSION "${PIPEWIRE_STRINGS}")
  set(MICRO_VERSION ${CMAKE_MATCH_1})
  set(PIPEWIRE_VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pipewire
                                  REQUIRED_VARS PIPEWIRE_LIBRARY PIPEWIRE_INCLUDE_DIR SPA_INCLUDE_DIR
                                  VERSION_VAR PIPEWIRE_VERSION_STRING)

if(PIPEWIRE_FOUND)
  set(PIPEWIRE_INCLUDE_DIRS ${PIPEWIRE_INCLUDE_DIR} ${SPA_INCLUDE_DIR})
  set(PIPEWIRE_LIBRARIES ${PIPEWIRE_LIBRARY})
  set(PIPEWIRE_DEFINITIONS -DHAS_PIPEWIRE=1)
  list(APPEND AUDIO_BACKENDS_LIST "pipewire")
  set(AUDIO_BACKENDS_LIST ${AUDIO_BACKENDS_LIST} PARENT_SCOPE)
endif()

mark_as_advanced(PIPEWIRE_INCLUDE_DIR PIPEWIRE_LIBRARY)
