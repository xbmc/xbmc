#.rst:
# FindZip
# -----------
# Finds the Zip library
#
# This will define the following variables::
#
# ZIP_FOUND - system has Zip
# ZIP_INCLUDE_DIRS - the Zip include directory
# ZIP_LIBRARIES - the Zip libraries
# ZIP_DEFINITIONS - the Zip libraries
#
# and the following imported targets::
#
#   ZIP::ZIP   - The Zip library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_ZIP libzip QUIET)
endif()

find_path(ZIP_INCLUDE_DIR zip.h
                          PATHS ${PC_ZIP_INCLUDEDIR})
find_library(ZIP_LIBRARY NAMES zip
                         PATHS ${PC_ZIP_LIBDIR})
set(ZIP_VERSION ${PC_ZIP_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Zip
                                  REQUIRED_VARS ZIP_LIBRARY ZIP_INCLUDE_DIR
                                  VERSION_VAR ZIP_VERSION)

if(ZIP_FOUND)
  set(ZIP_LIBRARIES ${ZIP_LIBRARY})
  set(ZIP_INCLUDE_DIRS ${ZIP_INCLUDE_DIR})
  set(ZIP_DEFINITIONS "${PC_ZIP_CFLAGS}")

  if(NOT TARGET ZIP::ZIP)
    add_library(ZIP::ZIP UNKNOWN IMPORTED)
    set_target_properties(ZIP::ZIP PROPERTIES
                                   IMPORTED_LOCATION "${ZIP_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${ZIP_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS "${PC_ZIP_CFLAGS}")
  endif()
endif()

mark_as_advanced(ZIP_INCLUDE_DIR ZIP_LIBRARY)
