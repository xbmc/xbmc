# - Try to find ASS
# Once done this will define
#
# ASS_FOUND - system has libass
# ASS_INCLUDE_DIRS - the libass include directory
# ASS_LIBRARIES - The libass libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (ASS libass)
else()
  find_path(ASS_INCLUDE_DIRS ass/ass.h)
  find_library(ASS_LIBRARIES NAMES ass libass)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ASS DEFAULT_MSG ASS_INCLUDE_DIRS ASS_LIBRARIES)

mark_as_advanced(ASS_INCLUDE_DIRS ASS_LIBRARIES)
