set(SOURCES PVRClients.cpp)

set(HEADERS PVRClients.h)

core_add_library(pvr_addons)
add_dependencies(pvr_addons libcpluff)
