# - Try to find IMXVPU
# Once done this will define
#
# IMXVPU_FOUND - system has IMXVPU
# IMXVPU_INCLUDE_DIRS - the IMXVPU include directory
# IMXVPU_LIBRARIES - The IMXVPU libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules(IMXVPU fslvpuwrap QUIET)
endif()

if(NOT IMXVPU_FOUND)
  find_path(IMXVPU_INCLUDE_DIRS imx-mm/vpu/vpu_wrapper.h)
  find_library(FSLVPUWRAP_LIBRARY fslvpuwrap)
  find_library(VPU_LIBRARY vpu)
  find_library(G2D_LIBRARY g2d)

  set(IMXVPU_LIBRARIES ${VPU_LIBRARY} ${FSLVPUWRAP_LIBRARY} ${G2D_LIBRARY}
      CACHE STRING "imx-vpu libraries" FORCE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IMXVPU DEFAULT_MSG IMXVPU_LIBRARIES IMXVPU_INCLUDE_DIRS)

list(APPEND IMXVPU_DEFINITIONS -DHAS_IMXVPU=1 -DLINUX -DEGL_API_FB)

mark_as_advanced(IMXVPU_INCLUDE_DIRS IMXVPU_LIBRARIES IMXVPU_DEFINITIONS)
