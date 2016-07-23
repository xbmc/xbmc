set(SOURCES UPnP.cpp
            UPnPInternal.cpp
            UPnPPlayer.cpp
            UPnPRenderer.cpp
            UPnPServer.cpp
            UPnPSettings.cpp)

set(HEADERS UPnP.h
            UPnPInternal.h
            UPnPPlayer.h
            UPnPRenderer.h
            UPnPServer.h
            UPnPSettings.h)

core_add_library(network_upnp)
if(ENABLE_STATIC_LIBS)
  target_link_libraries(network_upnp PRIVATE upnp)
endif()
