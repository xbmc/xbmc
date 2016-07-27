#.rst:
# FindJsonSchemaBuilder
# ---------------------
# Finds the JsonSchemaBuilder
#
# This will define the following (imported) targets::
#
#   JsonSchemaBuilder::JsonSchemaBuilder   - The JsonSchemaBuilder executable

if(NOT TARGET JsonSchemaBuilder::JsonSchemaBuilder)
  if(CMAKE_CROSSCOMPILING)
    add_executable(JsonSchemaBuilder::JsonSchemaBuilder IMPORTED GLOBAL)
    set_target_properties(JsonSchemaBuilder::JsonSchemaBuilder PROPERTIES
                                                               IMPORTED_LOCATION "${NATIVEPREFIX}/bin/JsonSchemaBuilder")
    set_target_properties(JsonSchemaBuilder::JsonSchemaBuilder PROPERTIES FOLDER Tools)
  else()
    add_subdirectory(${CORE_SOURCE_DIR}/tools/depends/native/JsonSchemaBuilder build/jsonschemabuilder)
    add_executable(JsonSchemaBuilder::JsonSchemaBuilder ALIAS JsonSchemaBuilder)
    set_target_properties(JsonSchemaBuilder PROPERTIES FOLDER Tools)
  endif()
endif()
