set(SOURCES cddb.cpp
            DNSNameCache.cpp
            EventClient.cpp
            EventPacket.cpp
            EventServer.cpp
            GUIDialogAccessPoints.cpp
            GUIDialogNetworkSetup.cpp
            Network.cpp
            NetworkServices.cpp
            Socket.cpp
            TCPServer.cpp
            UdpClient.cpp
            WakeOnAccess.cpp
            WebServer.cpp
            ZeroconfBrowser.cpp
            Zeroconf.cpp)

set(HEADERS cddb.h
            DNSNameCache.h
            EventClient.h
            EventPacket.h
            EventServer.h
            GUIDialogAccessPoints.h
            GUIDialogNetworkSetup.h
            Network.h
            NetworkServices.h
            Socket.h
            TCPServer.h
            UdpClient.h
            WakeOnAccess.h
            WebServer.h
            Zeroconf.h
            ZeroconfBrowser.h)

if(PLIST_FOUND)
  list(APPEND SOURCES AirPlayServer.cpp)
  list(APPEND HEADERS AirPlayServer.h
                      DllLibPlist.h)
endif()

if(SHAIRPLAY_FOUND)
  list(APPEND SOURCES AirTunesServer.cpp)
  list(APPEND HEADERS AirTunesServer.h
                      DllLibShairplay.h)
endif()

core_add_library(network)
