set(SOURCES TestNetwork.cpp)

if(MICROHTTPD_FOUND)
  list(APPEND SOURCES TestWebServer.cpp)
endif()

core_add_test_library(network_test)
