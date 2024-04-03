#.rst:
# FindJsonSchemaBuilder
# ---------------------
# Finds the JsonSchemaBuilder
#
# If WITH_JSONSCHEMABUILDER is defined and points to a directory,
# this path will be used to search for the JsonSchemaBuilder binary
#
#
# This will define the following (imported) targets::
#
#   JsonSchemaBuilder::JsonSchemaBuilder   - The JsonSchemaBuilder executable

if(NOT TARGET JsonSchemaBuilder::JsonSchemaBuilder)

  include(cmake/scripts/common/ModuleHelpers.cmake)

  if(WITH_JSONSCHEMABUILDER)
    get_filename_component(_jsbpath ${WITH_JSONSCHEMABUILDER} ABSOLUTE)
    get_filename_component(_jsbpath ${_jsbpath} DIRECTORY)
    find_program(JSONSCHEMABUILDER_EXECUTABLE NAMES "${APP_NAME_LC}-JsonSchemaBuilder" JsonSchemaBuilder
                                                    "${APP_NAME_LC}-JsonSchemaBuilder.exe" JsonSchemaBuilder.exe
                                              HINTS ${_jsbpath})

    if(NOT JSONSCHEMABUILDER_EXECUTABLE)
      message(FATAL_ERROR "Could not find 'JsonSchemaBuilder' executable in ${_jsbpath} supplied by -DWITH_JSONSCHEMABUILDER")
    endif()
  else()

    set(MODULE_LC JsonSchemaBuilder)
    set(${MODULE_LC}_LIB_TYPE native)
    set(JSONSCHEMABUILDER_DISABLE_VERSION ON)
    SETUP_BUILD_VARS()

    # Override build type detection and always build as release
    set(JSONSCHEMABUILDER_BUILD_TYPE Release)

    set(CMAKE_ARGS -DDUMMY_ARG=1)

    if(NATIVEPREFIX)
      set(INSTALL_DIR "${NATIVEPREFIX}/bin")
      set(JSONSCHEMABUILDER_INSTALL_PREFIX ${NATIVEPREFIX})
    else()
      set(INSTALL_DIR "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/bin")
      set(JSONSCHEMABUILDER_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR})
    endif()

    # Set host build info for buildtool
    if(EXISTS "${NATIVEPREFIX}/share/Toolchain-Native.cmake")
      set(JSONSCHEMABUILDER_TOOLCHAIN_FILE "${NATIVEPREFIX}/share/Toolchain-Native.cmake")
    endif()

    if(WIN32 OR WINDOWS_STORE)
      # Make sure we generate for host arch, not target
      set(JSONSCHEMABUILDER_GENERATOR_PLATFORM CMAKE_GENERATOR_PLATFORM ${HOSTTOOLSET})
      set(APP_EXTENSION ".exe")
    endif()

    set(JSONSCHEMABUILDER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/tools/depends/native/JsonSchemaBuilder/src)
    set(JSONSCHEMABUILDER_EXECUTABLE ${INSTALL_DIR}/JsonSchemaBuilder${APP_EXTENSION})

    set(BUILD_BYPRODUCTS ${JSONSCHEMABUILDER_EXECUTABLE})

    BUILD_DEP_TARGET()

  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(JsonSchemaBuilder
                                    REQUIRED_VARS JSONSCHEMABUILDER_EXECUTABLE)

  if(JSONSCHEMABUILDER_FOUND)
    add_executable(JsonSchemaBuilder::JsonSchemaBuilder IMPORTED GLOBAL)
    set_target_properties(JsonSchemaBuilder::JsonSchemaBuilder PROPERTIES
                                                               IMPORTED_LOCATION "${JSONSCHEMABUILDER_EXECUTABLE}")

    if(TARGET JsonSchemaBuilder)
      add_dependencies(JsonSchemaBuilder::JsonSchemaBuilder JsonSchemaBuilder)
    endif()
  else()
    if(JSONSCHEMABUILDER_FIND_REQUIRED)
      message(FATAL_ERROR "JsonSchemaBuilder not found.")
    endif()
  endif()

  mark_as_advanced(JSONSCHEMABUILDER_EXECUTABLE)
endif()
