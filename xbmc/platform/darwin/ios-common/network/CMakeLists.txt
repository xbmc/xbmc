set(SOURCES NetworkIOS.mm)

set(HEADERS NetworkIOS.h
            route.h)

set(UNITY_EXCLUDES ${SOURCES})

core_add_library(platform_ios_network)
