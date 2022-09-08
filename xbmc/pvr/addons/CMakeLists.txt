set(SOURCES PVRClient.cpp
            PVRClientCapabilities.cpp
            PVRClientMenuHooks.cpp
            PVRClientUID.cpp
            PVRClients.cpp)

set(HEADERS PVRClient.h
            PVRClientCapabilities.h
            PVRClientMenuHooks.h
            PVRClientUID.h
            PVRClients.h)

core_add_library(pvr_addons)
