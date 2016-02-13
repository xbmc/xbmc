if(MACOSX)
  list(APPEND SOURCES TestAESinkDARWINOSX.cpp)
endif()

if(SOURCES)
  core_add_test_library(audioengine_sink_test)
endif()
