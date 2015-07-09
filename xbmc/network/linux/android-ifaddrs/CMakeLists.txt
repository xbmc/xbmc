if(CORE_SYSTEM_NAME STREQUAL android)
    set(SOURCES ifaddrs.c)
    set(HEADERS ifaddrs.h)
    core_add_library(android_ifaddrs)
endif()

