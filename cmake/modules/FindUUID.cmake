#.rst:
# FindUUID
# --------
# Finds the libuuid library
#
# This will define the following target:
#
#   UUID::UUID   - The libuuid library
#

if(NOT TARGET UUID::UUID)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_UUID uuid QUIET)
  endif()

  find_path(UUID_INCLUDE_DIR uuid/uuid.h
                             HINTS ${PC_UUID_INCLUDEDIR})
  find_library(UUID_LIBRARY uuid
                            HINTS ${PC_UUID_LIBRARY})
  set(UUID_VERSION ${PC_UUID_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(UUID
                                    REQUIRED_VARS UUID_LIBRARY UUID_INCLUDE_DIR
                                    VERSION_VAR UUID_VERSION)

  if(UUID_FOUND)
    add_library(UUID::UUID UNKNOWN IMPORTED)
    set_target_properties(UUID::UUID PROPERTIES
                                     IMPORTED_LOCATION "${UUID_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${UUID_INCLUDE_DIR}")
  endif()
endif()
