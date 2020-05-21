if(AVAHI_FOUND)
  set(SOURCES ZeroconfAvahi.cpp
              ZeroconfBrowserAvahi.cpp)
  set(HEADERS ZeroconfAvahi.h
              ZeroconfBrowserAvahi.h)

  core_add_library(platform_linux_network_zeroconf)
endif()
