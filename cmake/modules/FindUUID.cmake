#.rst:
# FindUUID
# --------
# Finds the libuuid library
#
# This will will define the following variables::
#
# UUID_FOUND - system has libuuid
# UUID_INCLUDE_DIRS - the libuuid include directory
# UUID_LIBRARIES - the libuuid libraries
#
# and the following imported targets::
#
#   UUID::UUID   - The libuuid library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_UUID uuid QUIET)
endif()

find_path(UUID_INCLUDE_DIR uuid/uuid.h
                           PATHS ${PC_UUID_INCLUDEDIR})
find_library(UUID_LIBRARY uuid
                          PATHS ${PC_UUID_LIBRARY})
set(UUID_VERSION ${PC_UUID_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UUID
                                  REQUIRED_VARS UUID_LIBRARY UUID_INCLUDE_DIR
                                  VERSION_VAR UUID_VERSION)

if(UUID_FOUND)
  set(UUID_LIBRARIES ${UUID_LIBRARY})
  set(UUID_INCLUDE_DIRS ${UUID_INCLUDE_DIR})

  if(NOT TARGET UUID::UUID)
    add_library(UUID::UUID UNKNOWN IMPORTED)
    set_target_properties(UUID::UUID PROPERTIES
                                     IMPORTED_LOCATION "${UUID_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${UUID_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(UUID_INCLUDE_DIR UUID_LIBRARY)
