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
  if(KODI_DEPENDSBUILD)
    get_filename_component(_jsbpath "${NATIVEPREFIX}/bin" ABSOLUTE)
    find_program(JSONSCHEMABUILDER_EXECUTABLE NAMES "${APP_NAME_LC}-JsonSchemaBuilder" JsonSchemaBuilder
                                   HINTS ${_jsbpath})

    add_executable(JsonSchemaBuilder::JsonSchemaBuilder IMPORTED GLOBAL)
    set_target_properties(JsonSchemaBuilder::JsonSchemaBuilder PROPERTIES
                                                       IMPORTED_LOCATION "${JSONSCHEMABUILDER_EXECUTABLE}")
  elseif(CORE_SYSTEM_NAME STREQUAL windowsstore)
    get_filename_component(_jsbpath "${DEPENDENCIES_DIR}/bin/json-rpc" ABSOLUTE)
    find_program(JSONSCHEMABUILDER_EXECUTABLE NAMES "${APP_NAME_LC}-JsonSchemaBuilder" JsonSchemaBuilder
                                              HINTS ${_jsbpath})

    add_executable(JsonSchemaBuilder::JsonSchemaBuilder IMPORTED GLOBAL)
    set_target_properties(JsonSchemaBuilder::JsonSchemaBuilder PROPERTIES
                                                       IMPORTED_LOCATION "${JSONSCHEMABUILDER_EXECUTABLE}")
  else()
    if(WITH_JSONSCHEMABUILDER)
      get_filename_component(_jsbpath ${WITH_JSONSCHEMABUILDER} ABSOLUTE)
      get_filename_component(_jsbpath ${_jsbpath} DIRECTORY)
      find_program(JSONSCHEMABUILDER_EXECUTABLE NAMES "${APP_NAME_LC}-JsonSchemaBuilder" JsonSchemaBuilder
                                                HINTS ${_jsbpath})

      include(FindPackageHandleStandardArgs)
      find_package_handle_standard_args(JsonSchemaBuilder "Could not find '${APP_NAME_LC}-JsonSchemaBuilder' or 'JsonSchemaBuilder' executable in ${_jsbpath} supplied by -DWITH_JSONSCHEMABUILDER. Make sure the executable file name matches these names!"
                                        JSONSCHEMABUILDER_EXECUTABLE)
      if(JSONSCHEMABUILDER_FOUND)
        add_executable(JsonSchemaBuilder::JsonSchemaBuilder IMPORTED GLOBAL)
        set_target_properties(JsonSchemaBuilder::JsonSchemaBuilder PROPERTIES
                                                           IMPORTED_LOCATION "${JSONSCHEMABUILDER_EXECUTABLE}")
      endif()
      mark_as_advanced(JSONSCHEMABUILDER)
    else()
      add_subdirectory(${CMAKE_SOURCE_DIR}/tools/depends/native/JsonSchemaBuilder build/jsonschemabuilder)
      add_executable(JsonSchemaBuilder::JsonSchemaBuilder ALIAS JsonSchemaBuilder)
      set_target_properties(JsonSchemaBuilder PROPERTIES FOLDER Tools)
    endif()
  endif()
endif()
