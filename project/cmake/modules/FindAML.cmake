#.rst:
# FindAML
# -------
# Finds the AML codec
#
# This will will define the following variables::
#
# AML_FOUND - system has AML
# AML_INCLUDE_DIRS - the AML include directory
# AML_DEFINITIONS - the AML definitions
#
# and the following imported targets::
#
#   AML::AML   - The AML codec

find_path(AML_INCLUDE_DIR codec_error.h
                          PATH_SUFFIXES amcodec)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AML
                                  REQUIRED_VARS AML_INCLUDE_DIR)

if(AML_FOUND)
  set(AML_INCLUDE_DIRS ${AML_INCLUDE_DIR})
  set(AML_DEFINITIONS -DHAS_LIBAMCODEC=1)

  if(NOT TARGET AML::AML)
    add_library(AML::AML UNKNOWN IMPORTED)
    set_target_properties(AML::AML PROPERTIES
                                   INTERFACE_INCLUDE_DIRECTORIES "${AML_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAS_LIBAMCODEC=1)
  endif()
endif()

mark_as_advanced(AMLCODEC_INCLUDE_DIR)
