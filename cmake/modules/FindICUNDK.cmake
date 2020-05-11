# FindICUNDK
# --------
# Finds the ICUNDK library
#
# This will define the following variables::
#
# ICUNDK_FOUND - system has ICUNDK
# ICUNDK_INCLUDE_DIRS - the ICUNDK include directory
# ICUNDK_LIBRARIES - the ICUNDK libraries

find_path(ICUNDK_INCLUDE_DIR NAMES unicode/utypes.h)
find_library(ICUNDK_LIBRARY NAMES libicundk.a)
find_package_handle_standard_args(ICUNDK
                                  REQUIRED_VARS ICUNDK_INCLUDE_DIR ICUNDK_LIBRARY)

if (ICUNDK_FOUND)
  set(ICUNDK_INCLUDE_DIRS "${ICUNDK_INCLUDE_DIR}")
  set(ICUNDK_LIBRARIES "${ICUNDK_LIBRARY}")
endif()
