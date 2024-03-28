#.rst:
# FindUnicodeCLDR
# -------
# Populates resource.timezone with Windows to TZData timezone mapping
# file (WindowsZones.xml) from Unicode CLDR
#
# This will define the following variables:
#
# UNICODECLDR_FOUND   - system has internal timezone mapping file
# UNICODECLDR_VERSION - version of internal timezone mapping file

if(ENABLE_INTERNAL_TZDATA)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC unicode-cldr)

  SETUP_BUILD_VARS()

  # Mirror WindowsZones.xml to resource.timezone

  set(CMAKE_ARGS -DINSTALL_DIR=${CMAKE_BINARY_DIR}/addons/resource.timezone/resources/tzdata)

  # Add CMakeLists.txt installing sources as target

  set(patches ${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-cmakelists.patch)

  generate_patchcommand("${patches}")

  BUILD_DEP_TARGET()
else()
  set(UNICODECLDR_VERSION "none")
endif()

set(UNICODECLDR_FOUND TRUE)
