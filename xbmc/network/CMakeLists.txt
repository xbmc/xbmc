set(SOURCES DNSNameCache.cpp
            EventClient.cpp
            EventPacket.cpp
            EventServer.cpp
            GUIDialogNetworkSetup.cpp
            Network.cpp
            NetworkFileItemClassify.cpp
            NetworkServices.cpp
            Socket.cpp
            TCPServer.cpp
            UdpClient.cpp
            WakeOnAccess.cpp
            ZeroconfBrowser.cpp
            Zeroconf.cpp)

set(HEADERS DNSNameCache.h
            EventClient.h
            EventPacket.h
            EventServer.h
            GUIDialogNetworkSetup.h
            Network.h
            NetworkFileItemClassify.h
            NetworkServices.h
            Socket.h
            TCPServer.h
            UdpClient.h
            WakeOnAccess.h
            Zeroconf.h
            ZeroconfBrowser.h)

if(ENABLE_OPTICAL)
  list(APPEND SOURCES cddb.cpp)
  list(APPEND HEADERS cddb.h)
endif()

if(TARGET Plist::Plist)
  list(APPEND SOURCES AirPlayServer.cpp)
  list(APPEND HEADERS AirPlayServer.h)
endif()

if(TARGET Shairplay::Shairplay)
  list(APPEND SOURCES AirTunesServer.cpp)
  list(APPEND HEADERS AirTunesServer.h)
endif()

if(SMBCLIENT_FOUND)
  list(APPEND HEADERS IWSDiscovery.h)
endif()

if(MICROHTTPD_FOUND)
  list(APPEND SOURCES WebServer.cpp)
  list(APPEND HEADERS WebServer.h)
endif()

core_add_library(network)

if(ENABLE_STATIC_LIBS AND ENABLE_UPNP)
  target_link_libraries(${CORE_LIBRARY} PRIVATE upnp)
endif()
