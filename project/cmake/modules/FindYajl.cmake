# Base Io build system
# Written by Jeremy Tregunna <jeremy.tregunna@me.com>
#
# Find libyajl
pkg_check_modules(YAJL yajl>=2.0)
if(YAJL_FOUND)
  list(APPEND YAJL_DEFINITIONS -DYAJL_MAJOR=2)
endif()

if(NOT YAJL_FOUND)
  find_path(YAJL_INCLUDE_DIRS yajl/yajl_common.h)
  find_library(YAJL_LIBRARIES NAMES yajl)

  file(STRINGS ${YAJL_INCLUDE_DIRS}/yajl/yajl_version.h version_header)
  string(REGEX MATCH "YAJL_MAJOR ([0-9]+)" YAJL_VERSION_MAJOR ${version_header})
  string(REGEX REPLACE "YAJL_MAJOR ([0-9]+)" "\\1" YAJL_VERSION_MAJOR "${YAJL_VERSION_MAJOR}")
  if (YAJL_VERSION_MINOR LESS 2)
    set(YAJL_INCLUDE_DIRS)
    set(YALJ_LIBRARIES)
  endif()
  list(APPEND YAJL_DEFINITIONS -DYAJL_MAJOR=${YAJL_VERSION_MAJOR})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Yajl DEFAULT_MSG YAJL_INCLUDE_DIRS YAJL_LIBRARIES)

mark_as_advanced(YAJL_INCLUDE_DIRS YAJL_LIBRARIES YAJL_DEFINITIONS)
