#.rst:
# FindLibDRM
# ----------
# Finds the LibDRM library
#
# This will define the following variables::
#
# LIBDRM_FOUND - system has LibDRM
# LIBDRM_INCLUDE_DIRS - the LibDRM include directory
# LIBDRM_LIBRARIES - the LibDRM libraries
# LIBDRM_DEFINITIONS  - the LibDRM definitions
#
# and the following imported targets::
#
#   LibDRM::LibDRM   - The LibDRM library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_LIBDRM libdrm>=2.4.95 QUIET)
endif()

find_path(LIBDRM_INCLUDE_DIR NAMES drm.h
                             PATH_SUFFIXES libdrm drm
                             PATHS ${PC_LIBDRM_INCLUDEDIR})
find_library(LIBDRM_LIBRARY NAMES drm
                            PATHS ${PC_LIBDRM_LIBDIR})

set(LIBDRM_VERSION ${PC_LIBDRM_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibDRM
                                  REQUIRED_VARS LIBDRM_LIBRARY LIBDRM_INCLUDE_DIR
                                  VERSION_VAR LIBDRM_VERSION)

include(CheckCSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${LIBDRM_INCLUDE_DIR})
check_c_source_compiles("#include <drm_mode.h>

                         int main()
                         {
                           struct hdr_output_metadata test;
                           return test.metadata_type;
                         }
                         " LIBDRM_HAS_HDR_OUTPUT_METADATA)

if(LIBDRM_FOUND)
  set(LIBDRM_LIBRARIES ${LIBDRM_LIBRARY})
  set(LIBDRM_INCLUDE_DIRS ${LIBDRM_INCLUDE_DIR})
  if(LIBDRM_HAS_HDR_OUTPUT_METADATA)
    set(LIBDRM_DEFINITIONS -DHAVE_HDR_OUTPUT_METADATA=1)
  endif()

  if(NOT TARGET LIBDRM::LIBDRM)
    add_library(LIBDRM::LIBDRM UNKNOWN IMPORTED)
    set_target_properties(LIBDRM::LIBDRM PROPERTIES
                                   IMPORTED_LOCATION "${LIBDRM_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${LIBDRM_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(LIBDRM_INCLUDE_DIR LIBDRM_LIBRARY)
