#.rst:
# FindUdf
# --------
# Finds the udf library
#
# This will define the following variables::
#
# UDF_FOUND - system has udf
# UDF_INCLUDE_DIRS - the udf include directory
# UDF_LIBRARIES - the udf libraries
# UDF_DEFINITIONS - the udf definitions

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_UDF libudf>=0.94 QUIET)
endif()

find_path(UDF_INCLUDE_DIR NAMES cdio/udf.h
                          PATHS ${PC_UDF_INCLUDEDIR})

find_library(UDF_LIBRARY NAMES udf libudf
                         PATHS ${PC_UDF_LIBDIR})

set(UDF_VERSION ${PC_UDF_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Udf
                                  REQUIRED_VARS UDF_LIBRARY UDF_INCLUDE_DIR
                                  VERSION_VAR UDF_VERSION)

if(UDF_FOUND)
  set(UDF_LIBRARIES ${UDF_LIBRARY})
  set(UDF_INCLUDE_DIRS ${UDF_INCLUDE_DIR})
  set(UDF_DEFINITIONS -DHAS_UDF=1)
endif()

mark_as_advanced(UDF_INCLUDE_DIR UDF_LIBRARY)
