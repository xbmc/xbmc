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

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/addons/xbmc.json/addon.xml
                   COMMAND ${CMAKE_COMMAND}
                           -DCORE_SOURCE_DIR=${CORE_SOURCE_DIR}
                           -DCORE_BINARY_DIR=${CMAKE_BINARY_DIR}
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/GenerateAddonXml.cmake
                   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                   DEPENDS ${JSON_SRCS} ${CORE_SOURCE_DIR}/addons/xbmc.json/addon.xml.in
                   COMMENT "Generating xbmc.json/addon.xml")

add_custom_target(generate_json_header ALL
                  DEPENDS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ServiceDescription.h
                          ${CMAKE_BINARY_DIR}/addons/xbmc.json/addon.xml)
set_target_properties(generate_json_header PROPERTIES FOLDER "Build Utilities")

if(BOOTSTRAP_IN_TREE)
  add_dependencies(generate_json_header JsonSchemaBuilder::JsonSchemaBuilder)
endif()
