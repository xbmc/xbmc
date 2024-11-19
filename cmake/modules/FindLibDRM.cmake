#.rst:
# FindLibDRM
# ----------
# Finds the LibDRM library
#
# This will define the following target:
#
#   LibDRM::LibDRM   - The LibDRM library

if(NOT TARGET LIBDRM::LIBDRM)

  if(LibDRM_FIND_VERSION)
    if(LibDRM_FIND_VERSION_EXACT)
      set(LibDRM_FIND_SPEC "=${LibDRM_FIND_VERSION_COMPLETE}")
    else()
      set(LibDRM_FIND_SPEC ">=${LibDRM_FIND_VERSION_COMPLETE}")
    endif()
  endif()

  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBDRM libdrm${LibDRM_FIND_SPEC} QUIET)
  endif()

  find_path(LIBDRM_INCLUDE_DIR NAMES drm.h
                               PATH_SUFFIXES libdrm drm
                               HINTS ${PC_LIBDRM_INCLUDEDIR}
                               NO_CACHE)
  find_library(LIBDRM_LIBRARY NAMES drm
                              HINTS ${PC_LIBDRM_LIBDIR}
                              NO_CACHE)

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

  include(CheckSymbolExists)
  set(CMAKE_REQUIRED_LIBRARIES ${LIBDRM_LIBRARY})
  check_symbol_exists(drmGetFormatModifierName xf86drm.h LIBDRM_HAS_MODIFIER_NAME)

  if(LIBDRM_FOUND)
    add_library(LIBDRM::LIBDRM UNKNOWN IMPORTED)
    set_target_properties(LIBDRM::LIBDRM PROPERTIES
                                         IMPORTED_LOCATION "${LIBDRM_LIBRARY}"
                                         INTERFACE_INCLUDE_DIRECTORIES "${LIBDRM_INCLUDE_DIR}")
    if(LIBDRM_HAS_HDR_OUTPUT_METADATA)
      set_property(TARGET LIBDRM::LIBDRM APPEND PROPERTY
                                                INTERFACE_COMPILE_DEFINITIONS HAVE_HDR_OUTPUT_METADATA=1)
    endif()
    if(LIBDRM_HAS_MODIFIER_NAME)
      set_property(TARGET LIBDRM::LIBDRM APPEND PROPERTY
                                                INTERFACE_COMPILE_DEFINITIONS HAVE_DRM_MODIFIER_NAME=1)
    endif()
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LIBDRM::LIBDRM)
  endif()
endif()
