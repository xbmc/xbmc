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

if(PLATINUM_FOUND)
  core_add_library(network_upnp)
endif()
