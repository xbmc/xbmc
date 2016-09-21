# - Try to find RPI
# Once done this will define
#
# RPI_FOUND - system has RPI
# RPI_INCLUDE_DIRS - the RPI include directory
# RPI_LIBRARIES - The RPI libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules(RPI mmal QUIET)
endif()

if(NOT RPI_FOUND)
  find_path(MMAL_INCLUDE_DIRS interface/mmal/mmal.h)
  find_library(MMAL_LIBRARY  mmal)
  find_library(MMALCORE_LIBRARY mmal_core)
  find_library(MMALUTIL_LIBRARY mmal_util)
  find_library(MMALCLIENT_LIBRARY mmal_vc_client)
  find_library(MMALCOMPONENT_LIBRARY mmal_components)
  find_library(BCM_LIBRARY bcm_host)
  find_library(VCHIQ_LIBRARY vchiq_arm)
  find_library(VCOS_LIBRARY vcos)
  find_library(VCSM_LIBRARY vcsm)
  find_library(CONTAINER_LIBRARY containers)

  set(RPI_LIBRARIES ${MMAL_LIBRARY} ${MMALCORE_LIBRARY} ${MMALUTIL_LIBRARY}
                     ${MMALCLIENT_LIBRARY} ${MMALCOMPONENT_LIBRARY}
                     ${BCM_LIBRARY} ${VCHIQ_LIBRARY} ${VCOS_LIBRARY} ${VCSM_LIBRARY} ${CONTAINER_LIBRARY}
      CACHE STRING "RPi support libraries" FORCE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RPI DEFAULT_MSG RPI_LIBRARIES MMAL_INCLUDE_DIRS)

list(APPEND RPI_DEFINITIONS -DHAS_MMAL=1 -DHAS_OMXPLAYER -DHAVE_OMXLIB)

mark_as_advanced(MMAL_INCLUDE_DIRS MMAL_LIBRARIES MMAL_DEFINITIONS)
