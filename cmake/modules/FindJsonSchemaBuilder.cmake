#.rst:
# FindJsonSchemaBuilder
# ---------------------
# Finds the JsonSchemaBuilder
#
# This will define the following (imported) targets::
#
#   JsonSchemaBuilder::JsonSchemaBuilder   - The JsonSchemaBuilder executable

if(NOT TARGET JsonSchemaBuilder::JsonSchemaBuilder)
  if(KODI_DEPENDSBUILD OR CMAKE_CROSSCOMPILING)
    add_executable(JsonSchemaBuilder::JsonSchemaBuilder IMPORTED GLOBAL)
    if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
      set_target_properties(JsonSchemaBuilder::JsonSchemaBuilder PROPERTIES
                                                                 IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/project/BuildDependencies/bin/json-rpc/JsonSchemaBuilder")
    else()
      set_target_properties(JsonSchemaBuilder::JsonSchemaBuilder PROPERTIES
                                                                 IMPORTED_LOCATION "${NATIVEPREFIX}/bin/JsonSchemaBuilder")
    endif()
    set_target_properties(JsonSchemaBuilder::JsonSchemaBuilder PROPERTIES FOLDER Tools)
  else()
    add_subdirectory(${CMAKE_SOURCE_DIR}/tools/depends/native/JsonSchemaBuilder build/jsonschemabuilder)
    add_executable(JsonSchemaBuilder::JsonSchemaBuilder ALIAS JsonSchemaBuilder)
    set_target_properties(JsonSchemaBuilder PROPERTIES FOLDER Tools)
  endif()
endif()
