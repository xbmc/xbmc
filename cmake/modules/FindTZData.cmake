#.rst:
# FindTZData
# -------
# Populates resource.timezone with TZDATA if requested
#
# This will define the following variables::
#
# TZDATA_FOUND   - system has internal TZDATA
# TZDATA_VERSION - version of internal TZDATA

if(ENABLE_INTERNAL_TZDATA)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC tzdata)

  SETUP_BUILD_VARS()

  # Mirror tzdata to resource.timezone

  set(CMAKE_ARGS -DINSTALL_DIR=${CMAKE_BINARY_DIR}/addons/resource.timezone/resources/tzdata)

  # Add CMakeLists.txt installing sources as target

  set(patches ${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-cmakelists.patch)

  generate_patchcommand("${patches}")

  BUILD_DEP_TARGET()
else()
  set(TZDATA_VERSION "none")
endif()

set(TZDATA_FOUND TRUE)
