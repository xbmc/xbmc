# - Try to find MMAL
# Once done this will define
#
# MMAL_FOUND - system has MMAL
# MMAL_INCLUDE_DIRS - the MMAL include directory
# MMAL_LIBRARIES - The MMAL libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (MMAL mmal)
endif()

if(NOT MMAL_FOUND)
  find_path(MMAL_INCLUDE_DIRS interface/mmal/mmal.h)
  find_library(MMAL_LIBRARY  mmal)
  find_library(MMALCORE_LIBRARY mmal_core)
  find_library(MMALUTIL_LIBRARY mmal_util)
  find_library(MMALCLIENT_LIBRARY mmal_vc_client)
  find_library(MMALCOMPONENT_LIBRARY mmal_components)
  find_library(BCM_LIBRARY bcm_host)
  find_library(VCHIQ_LIBRARY vchiq_arm)
  find_library(VCOS_LIBRARY vcos)

  set(MMAL_LIBRARIES ${MMAL_LIBRARY} ${MMALCORE_LIBRARY} ${MMALUTIL_LIBRARY}
                     ${MMALCLIENT_LIBRARY} ${MMALCOMPONENT_LIBRARY}
                     ${BCM_LIBRARY} ${VCHIQ_LIBRARY} ${VCOS_LIBRARY}
      CACHE STRING "mmal libraries" FORCE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MMAL DEFAULT_MSG MMAL_INCLUDE_DIRS MMAL_LIBRARIES)

list(APPEND MMAL_DEFINITIONS -DHAVE_MMAL=1 -DHAS_MMAL=1)

mark_as_advanced(MMAL_INCLUDE_DIRS MMAL_LIBRARIES MMAL_DEFINITIONS)
