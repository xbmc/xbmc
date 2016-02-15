# - Try to find XSLT
# Once done this will define
#
# XSLT_FOUND - system has libxslt
# XSLT_INCLUDE_DIRS - the libxslt include directory
# XSLT_LIBRARIES - The libxslt libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (XSLT libxslt)
else()
  find_path(XSLT_INCLUDE_DIRS libxslt/xslt.h)
  find_library(XSLT_LIBRARIES NAMES xslt libxslt)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Xslt DEFAULT_MSG XSLT_INCLUDE_DIRS XSLT_LIBRARIES)

mark_as_advanced(XSLT_INCLUDE_DIRS XSLT_LIBRARIES)
