set(SOURCES PVRClient.cpp
            PVRClients.cpp)

set(HEADERS PVRClient.h
            PVRClients.h)

core_add_library(pvr_addons)
add_dependencies(pvr_addons libcpluff)
