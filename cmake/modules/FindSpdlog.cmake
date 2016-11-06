# FindSpdlog
# -------
# Finds the Spdlog library
#
# This will define the following variables::
#
# SPDLOG_FOUND - system has Spdlog
# SPDLOG_INCLUDE_DIRS - the Spdlog include directory
# SPDLOG_LIBRARIES - the Spdlog libraries
# SPDLOG_DEFINITIONS - the Spdlog compile definitions
#
# and the following imported targets::
#
#   Spdlog::Spdlog   - The Spdlog library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SPDLOG spdlog QUIET)
endif()

find_path(SPDLOG_INCLUDE_DIR NAMES spdlog/spdlog.h
                             PATHS ${PC_SPDLOG_INCLUDEDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Spdlog
                                  REQUIRED_VARS SPDLOG_INCLUDE_DIR)

if(SPDLOG_FOUND)
  set(SPDLOG_INCLUDE_DIRS ${SPDLOG_INCLUDE_DIR})
  set(SPDLOG_DEFINITIONS -DSPDLOG_FMT_EXTERNAL -DSPDLOG_DEBUG_ON)
  if(WIN32)
    list(APPEND SPDLOG_DEFINITIONS -DSPDLOG_WCHAR_FILENAMES)
  endif()

  if(NOT TARGET Spdlog::Spdlog)
    add_library(Spdlog::Spdlog UNKNOWN IMPORTED)
    set_target_properties(Spdlog::Spdlog PROPERTIES
                                         INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIR}"
                                         INTERFACE_COMPILE_DEFINITIONS "${SPDLOG_DEFINITIONS}")
  endif()
endif()

mark_as_advanced(SPDLOG_INCLUDE_DIR)
