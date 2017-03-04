#.rst:
# FindROCKCHIP
# ---------
# Finds the ROCKCHIP library
#
# This will will define the following variables::
#
# ROCKCHIP_FOUND - system has ROCKCHIP
# ROCKCHIP_INCLUDE_DIRS - the ROCKCHIP include directory
# ROCKCHIP_LIBRARIES - the ROCKCHIP libraries
#
# and the following imported targets::
#
#   ROCKCHIP::ROCKCHIP   - The ROCKCHIP library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_ROCKCHIP rockchip_mpp rockchip_vpu QUIET)
endif()

find_path(ROCKCHIP_INCLUDE_DIR NAMES rockchip/vpu_api.h
                                PATHS ${PC_ROCKCHIP_INCLUDEDIR})
find_library(ROCKCHIP_LIBRARY NAMES rockchip_mpp rockchip_vpu
                               PATHS ${PC_ROCKCHIP_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ROCKCHIP
                                  REQUIRED_VARS ROCKCHIP_LIBRARY ROCKCHIP_INCLUDE_DIR
                                  VERSION_VAR ROCKCHIP_VERSION)

if(ROCKCHIP_FOUND)
  set(ROCKCHIP_INCLUDE_DIRS ${ROCKCHIP_INCLUDE_DIR})
  set(ROCKCHIP_LIBRARIES ${ROCKCHIP_LIBRARY})
  set(ROCKCHIP_DEFINITIONS -DHAVE_ROCKCHIP=1 -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES -fpermissive -O2)

  if(NOT TARGET ROCKCHIP::ROCKCHIP)
    add_library(ROCKCHIP::ROCKCHIP UNKNOWN IMPORTED)
    set_target_properties(ROCKCHIP::ROCKCHIP PROPERTIES
                                               IMPORTED_LOCATION "${ROCKCHIP_LIBRARY}"
                                               INTERFACE_INCLUDE_DIRECTORIES "${ROCKCHIP_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(ROCKCHIP_INCLUDE_DIR ROCKCHIP_LIBRARY)
