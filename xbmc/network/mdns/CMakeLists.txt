if(TARGET MDNS::MDNS)
  set(SOURCES ZeroconfBrowserMDNS.cpp
              ZeroconfMDNS.cpp)

  set(HEADERS ZeroconfBrowserMDNS.h
              ZeroconfMDNS.h)

  core_add_library(network_mdns)
endif()
