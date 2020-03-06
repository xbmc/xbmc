#.rst:
# FindIso9660pp
# --------
# Finds the iso9660++ library
#
# This will define the following variables::
#
# ISO9660PP_FOUND - system has iso9660++
# ISO9660PP_INCLUDE_DIRS - the iso9660++ include directory
# ISO9660PP_LIBRARIES - the iso9660++ libraries
# ISO9660PP_DEFINITIONS  - the iso9660++ definitions

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_ISO9660PP libiso9660++>=2.1.0 QUIET)
  pkg_check_modules(PC_ISO9660 libiso9660>=2.1.0 QUIET)
endif()

find_path(ISO9660PP_INCLUDE_DIR NAMES cdio++/iso9660.hpp
                                PATHS ${PC_ISO9660PP_INCLUDEDIR})

find_library(ISO9660PP_LIBRARY NAMES libiso9660++ iso9660++
                               PATHS ${PC_ISO9660PP_LIBDIR})

find_path(ISO9660_INCLUDE_DIR NAMES cdio/iso9660.h
                              PATHS ${PC_ISO9660_INCLUDEDIR})

find_library(ISO9660_LIBRARY NAMES libiso9660 iso9660
                             PATHS ${PC_ISO9660_LIBDIR})

set(ISO9660PP_VERSION ${PC_ISO9660PP_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Iso9660pp
                                  REQUIRED_VARS ISO9660PP_LIBRARY ISO9660PP_INCLUDE_DIR ISO9660_LIBRARY ISO9660_INCLUDE_DIR
                                  VERSION_VAR ISO9660PP_VERSION)

if(ISO9660PP_FOUND)
  set(ISO9660PP_LIBRARIES ${ISO9660PP_LIBRARY} ${ISO9660_LIBRARY})
  set(ISO9660PP_INCLUDE_DIRS ${ISO9660PP_INCLUDE_DIR} ${ISO9660_INCLUDE_DIR})
  set(ISO9660PP_DEFINITIONS -DHAS_ISO9660PP=1)
endif()

mark_as_advanced(ISO9660PP_INCLUDE_DIR ISO9660PP_LIBRARY ISO9660_INCLUDE_DIR ISO9660_LIBRARY)
