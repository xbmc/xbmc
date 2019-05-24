set(SOURCES DNSNameCache.cpp
            EventClient.cpp
            EventPacket.cpp
            EventServer.cpp
            GUIDialogNetworkSetup.cpp
            Network.cpp
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

if(MICROHTTPD_FOUND)
  list(APPEND SOURCES WebServer.cpp)
  list(APPEND HEADERS WebServer.h)
endif()

core_add_library(network)
if(BLUETOOTH_FOUND)
  target_compile_definitions(${CORE_LIBRARY} PRIVATE -DHAVE_LIBBLUETOOTH=1)
endif()

if(ENABLE_UPNP)
  target_include_directories(${CORE_LIBRARY} PRIVATE ${CMAKE_SOURCE_DIR}/lib/libUPnP
                                                     ${CMAKE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Core
                                                     ${CMAKE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Platinum
                                                     ${CMAKE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Devices/MediaConnect
                                                     ${CMAKE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Devices/MediaRenderer
                                                     ${CMAKE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Devices/MediaServer
                                                     ${CMAKE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Extras
                                                     ${CMAKE_SOURCE_DIR}/lib/libUPnP/Neptune/Source/System/Posix
                                                     ${CMAKE_SOURCE_DIR}/lib/libUPnP/Neptune/Source/Core)
endif()
