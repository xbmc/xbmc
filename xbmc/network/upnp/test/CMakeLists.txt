set(SOURCES TestUPnPInternal.cpp)

core_add_test_library(network_upnp_test)
if(ENABLE_STATIC_LIBS)
  target_link_libraries(network_upnp_test PRIVATE upnp)
endif()
