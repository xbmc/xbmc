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

include_directories(${CORE_SOURCE_DIR}/lib/libUPnP
                    ${CORE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Core
                    ${CORE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Platinum
                    ${CORE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Devices/MediaConnect
                    ${CORE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Devices/MediaRenderer
                    ${CORE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Devices/MediaServer
                    ${CORE_SOURCE_DIR}/lib/libUPnP/Platinum/Source/Extras
                    ${CORE_SOURCE_DIR}/lib/libUPnP/Neptune/Source/System/Posix
                    ${CORE_SOURCE_DIR}/lib/libUPnP/Neptune/Source/Core)

add_definitions(-DNPT_CONFIG_ENABLE_LOGGING)

core_add_library(network_upnp)
