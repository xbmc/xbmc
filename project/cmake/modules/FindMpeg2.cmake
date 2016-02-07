# - Try to find MPEG2
# Once done this will define
#
# MPEG2_FOUND - system has libMPEG2
# MPEG2_INCLUDE_DIRS - the libMPEG2 include directory
# MPEG2_LIBRARIES - The libMPEG2 libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (MPEG2 libmpeg2)
else()
  find_package(MPEG2)
  set(MPEG2_INCLUDE_DIRS ${MPEG2_INCLUDE_DIR})
  set(MPEG2_LIBRARIES ${MPEG2_mpeg2_LIBRARIES})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mpeg2 DEFAULT_MSG MPEG2_INCLUDE_DIRS MPEG2_LIBRARIES)

mark_as_advanced(MPEG2_INCLUDE_DIRS MPEG2_LIBRARIES)
