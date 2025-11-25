#.rst:
# FindONNXRuntime
# ---------------
# Finds the ONNX Runtime library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::ONNXRuntime   - The ONNX Runtime library
#
# This module will look for ONNX Runtime 1.16.0 or higher.
# ONNX Runtime is used for local embedding inference using ONNX models.

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC onnxruntime)

  SETUP_BUILD_VARS()

  # Set minimum version requirement
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_MIN_VERSION "1.16.0")

  # Look for ONNX Runtime using pkg-config first
  SETUP_FIND_SPECS()
  SEARCH_EXISTING_PACKAGES()

  # If pkg-config didn't find it, try manual search
  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    # Try to find ONNX Runtime manually
    find_path(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
        NAMES onnxruntime_cxx_api.h
        PATH_SUFFIXES onnxruntime
                      onnxruntime/core/session
        HINTS ${DEPENDS_PATH}/include
    )

    find_library(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY
        NAMES onnxruntime
        HINTS ${DEPENDS_PATH}/lib
    )

    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR AND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY)
      set(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND TRUE)
    endif()
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    # If found via pkg-config, create alias
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      # Create imported target for manual find
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
          IMPORTED_LOCATION "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}"
      )
    endif()

    # Set compile definition to indicate ONNX Runtime is available
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_ONNXRUNTIME)
    ADD_TARGET_COMPILE_DEFINITION()

    message(STATUS "Found ONNX Runtime")
  else()
    if(ONNXRuntime_FIND_REQUIRED)
      message(FATAL_ERROR "ONNX Runtime library was not found. "
                          "Install ONNX Runtime 1.16.0+ or disable semantic features. "
                          "See https://github.com/microsoft/onnxruntime/releases")
    else()
      message(STATUS "ONNX Runtime not found - semantic embedding features will be disabled")
    endif()
  endif()
endif()
