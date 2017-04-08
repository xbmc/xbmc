if(MICROHTTPD_FOUND)
  set(SOURCES TestWebServer.cpp)

  core_add_test_library(network_test)
endif()
