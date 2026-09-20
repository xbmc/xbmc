set(SOURCES TestSavestateDiscState.cpp)

core_add_test_library(retroplayer_savestates_test)

if(ENABLE_STATIC_LIBS)
  add_dependencies(retroplayer_savestates_test retroplayer_messages)
  target_link_libraries(retroplayer_savestates_test PRIVATE ${APP_NAME_LC}::FlatBuffers)
endif()
