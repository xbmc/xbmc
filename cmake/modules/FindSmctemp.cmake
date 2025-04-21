#.rst:
# FindSmctemp
# -------
# Finds the smctemp library
#
# This will define the following imported targets::
#
#   ${APP_NAME_LC}::Smctemp   - The smctemp library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC smctemp)

  SETUP_FIND_SPECS()

  find_path(SMCTEMP_INCLUDE_DIR NAMES smctemp.h
                                HINTS ${DEPENDS_PATH}/include)

  if(EXISTS ${SMCTEMP_INCLUDE_DIR}/smctemp.h)
    file(STRINGS ${SMCTEMP_INCLUDE_DIR}/smctemp.h _smc_version_str
         REGEX ".*kVersion\[\] = \"[0-9.]+\".*")
    string(REGEX REPLACE ".*kVersion.*[\"](.*)[\"].*" "\\1" SMCTEMP_VERSION ${_smc_version_str})
    unset(_smc_version_str)

    if(SMCTEMP_VERSION VERSION_GREATER_EQUAL ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC})
      find_library(SMCTEMP_LIBRARY NAMES smctemp
                                   HINTS ${DEPENDS_PATH}/lib)
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Smctemp
                                    REQUIRED_VARS SMCTEMP_LIBRARY SMCTEMP_INCLUDE_DIR
                                    VERSION_VAR SMCTEMP_VERSION)

  if(SMCTEMP_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${SMCTEMP_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${SMCTEMP_INCLUDE_DIR}")
  else()
    if(Smctemp_FIND_REQUIRED)
      message(FATAL_ERROR "Smctemp library not found.")
    endif()
  endif()
endif()
