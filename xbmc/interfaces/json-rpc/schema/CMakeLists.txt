set(JSON_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/version.txt
              ${CMAKE_CURRENT_SOURCE_DIR}/license.txt
              ${CMAKE_CURRENT_SOURCE_DIR}/methods.json
              ${CMAKE_CURRENT_SOURCE_DIR}/types.json
              ${CMAKE_CURRENT_SOURCE_DIR}/notifications.json)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ServiceDescription.h
                   COMMAND JsonSchemaBuilder::JsonSchemaBuilder ${JSON_SRCS}
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                   DEPENDS ${JSON_SRCS}
                   COMMENT "Generating ServiceDescription.h")

if(${CORE_SYSTEM_NAME} MATCHES "windows")
  set(CORE_BINARY_DIR $<TARGET_FILE_DIR:${APP_NAME_LC}>)
else()
  set(CORE_BINARY_DIR ${CMAKE_BINARY_DIR})
endif()

add_custom_target(generate_json_header ALL
                  DEPENDS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ServiceDescription.h)

add_custom_command(TARGET generate_json_header
                   PRE_BUILD
                   COMMAND ${CMAKE_COMMAND}
                           -DCMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                           -DCORE_BINARY_DIR=${CORE_BINARY_DIR}
                           -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/GenerateAddonXml.cmake
                   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                   COMMENT "Generating xbmc.json/addon.xml")

set_target_properties(generate_json_header PROPERTIES FOLDER "Build Utilities")

# We require this for lib target.
add_dependencies(lib${APP_NAME_LC} generate_json_header)

if(BOOTSTRAP_IN_TREE)
  add_dependencies(generate_json_header JsonSchemaBuilder::JsonSchemaBuilder)
endif()
