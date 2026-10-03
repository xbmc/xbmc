set(SOURCES TestDNSNameCache.cpp
            TestNetwork.cpp
            TestNetworkFileItemClassify.cpp)

if(TARGET ${APP_NAME_LC}::MicroHttpd)
  list(APPEND SOURCES TestWebServer.cpp)
endif()

core_add_test_library(network_test)
