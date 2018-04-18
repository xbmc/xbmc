# FindXkbcommon
# -----------
# Finds the libxkbcommon library
#
# This will define the following variables::
#
# XKBCOMMON_FOUND        - the system has libxkbcommon
# XKBCOMMON_INCLUDE_DIRS - the libxkbcommon include directory
# XKBCOMMON_LIBRARIES    - the libxkbcommon libraries
# XKBCOMMON_DEFINITIONS  - the libxkbcommon definitions

pkg_check_modules (XKBCOMMON xkbcommon)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (Xkbcommon
  REQUIRED_VARS
  XKBCOMMON_FOUND)

set(XKBCOMMON_DEFINITIONS -DHAVE_XKBCOMMON=1)
set(XKBCOMMON_LIBRARIES ${XKBCOMMON_LDFLAGS})
set(XKBCOMMON_INCLUDE_DIRS ${XKBCOMMON_INCLUDEDIR})
