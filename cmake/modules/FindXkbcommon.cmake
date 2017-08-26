# FindXkbcommon
# -----------
# Finds the libxkbcommon library
#
# This will will define the following variables::
#
# XKBCOMMON_FOUND        - the system has libxkbcommon
# XKBCOMMON_INCLUDE_DIRS - the libxkbcommon include directory
# XKBCOMMON_LIBRARIES    - the libxkbcommon libraries
# XKBCOMMON_DEFINITIONS  - the libxkbcommon definitions


if(PKG_CONFIG_FOUND)
  pkg_check_modules (PC_XKBCOMMON xkbcommon QUIET)
endif()

find_path(XKBCOMMON_INCLUDE_DIR NAMES xkbcommon/xkbcommon.h
                                PATHS ${PC_XKBCOMMON_INCLUDE_DIRS})

find_library(XKBCOMMON_LIBRARY NAMES xkbcommon
                               PATHS ${PC_XKBCOMMON_LIBRARIES} ${PC_XKBCOMMON_LIBRARY_DIRS})

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (XKBCOMMON
  REQUIRED_VARS
  XKBCOMMON_INCLUDE_DIR
  XKBCOMMON_LIBRARY)

if (XKBCOMMON_FOUND)
  set(XKBCOMMON_LIBRARIES ${XKBCOMMON_LIBRARY})
  set(XKBCOMMON_INCLUDE_DIRS ${PC_XKBCOMMON_INCLUDE_DIRS})
  set(XKBCOMMON_DEFINITIONS -DHAVE_XKBCOMMON=1)
endif()

mark_as_advanced (XKBCOMMON_LIBRARY XKBCOMMON_INCLUDE_DIR)