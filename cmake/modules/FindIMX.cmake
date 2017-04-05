#.rst:
# FindIMX
# -------
# Finds the IMX codec
#
# This will will define the following variables::
#
# IMX_FOUND - system has IMX
# IMX_INCLUDE_DIRS - the IMX include directory
# IMX_DEFINITIONS - the IMX definitions
# IMX_LIBRARIES - the IMX libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules(IMX fslvpuwrap QUIET)
endif()

find_path(IMX_INCLUDE_DIR NAMES vpu_wrapper.h
                          PATH_SUFFIXES imx-mm/vpu
                          PATHS ${PC_IMX_INCLUDEDIR})

find_library(FSLVPUWRAP_LIBRARY NAMES fslvpuwrap
                                PATHS ${PC_IMX_LIBDIR})
find_library(VPU_LIBRARY NAMES vpu
                         PATHS ${PC_IMX_LIBDIR})
find_library(G2D_LIBRARY NAMES g2d
                         PATHS ${PC_IMX_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IMX
                                  REQUIRED_VARS IMX_INCLUDE_DIR FSLVPUWRAP_LIBRARY VPU_LIBRARY G2D_LIBRARY)

if(IMX_FOUND)
  set(IMX_INCLUDE_DIRS ${IMX_INCLUDE_DIR})
  set(IMX_LIBRARIES ${FSLVPUWRAP_LIBRARY} ${VPU_LIBRARY} ${G2D_LIBRARY})
  set(IMX_DEFINITIONS -DHAS_IMXVPU=1 -DLINUX -DEGL_API_FB)
endif()

mark_as_advanced(IMX_INCLUDE_DIR FSLVPUWRAP_LIBRARY VPU_LIBRARY G2D_LIBRARY)
