set(MESSAGES savestate.fbs
             video.fbs
)

foreach(_file ${MESSAGES})
  get_filename_component(FLATC_OUTPUT ${_file} NAME_WE)
  set(FLATC_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FLATC_OUTPUT}_generated.h)
  list(APPEND FLATC_OUTPUTS ${FLATC_OUTPUT})

  add_custom_command(OUTPUT ${FLATC_OUTPUT}
                     COMMAND flatbuffers::flatc
                     ARGS -c -o "${CMAKE_CURRENT_BINARY_DIR}/" ${_file}
                     DEPENDS ${_file}
                     COMMENT "Building C++ header for ${_file}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

add_custom_target(retroplayer_messages DEPENDS ${FLATC_OUTPUTS})
set_target_properties(retroplayer_messages PROPERTIES FOLDER "Generated Messages"
                                                      INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}
                                                      SOURCES "${FLATC_OUTPUTS}")

if(TARGET flatbuffers::flatheaders)
  set_property(TARGET flatbuffers::flatheaders APPEND PROPERTY
                                               INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}")
  add_dependencies(retroplayer_messages flatbuffers::flatheaders)
endif()
